# Attack Lab

## recitation

分成两个part，第一个是buffer overflow，第二个是ROP attacks



### 寄存器复习

---

#### caller-saved

\$rdx、\$rcx、\$rdi、\$rsi、\$r8、\$r9、\$rax、\$r10、\$r11

#### callee-saved

\$r12-\$15、\$rbx、\$rbp、\$rsp



**call**：\$rsp-8，放入\$rip，更新\$rip

**ret**：更新\$rip，\$rsp+8

**push**：\$rsp-8，将\$reg放入(\$rsp)



### Stack Frame布局

---

每个Function都有Stack Frame

- local Variables
- Callee-saved and caller-saved \$reg
- extra args

From high addr to low addr, there is 

- Saved \$regs
- local vars
- args to be passed to inner function



### 背景

---

ctarget和rtarget通过`getbuf`从stdin读取字符串，`getbuf`内部通过`Gets`读取字符串，`Gets`有可能会造成buffer overflow。



### hex2raw使用

---

> 为什么需要hex2raw？

最终在内存中存放的应当是一个一个的byte，但是我们需要以string的形式输入，如果没有hex2raw，则需要自己进行这层翻译，然后输入，而有了hex2raw，我们输入的就是最终内存中实际存放的bytes seq。



### phase1-3

---

缓冲区溢出有可能导致stack上的局部变量（例如return addr）被破坏，甚至运行一些exploit code。phase1-3通过输入的字符串进行code injection。

#### phase1

无需进行code injection，只需要通过buffer overflow修改return addr。

运行顺序是：调用`test`，内部调用`getbuf`，理想情况是在`getbuf`在返回时，非正常返回，而是返回至修改过的addr，这个addr通过input string修改。

`touch1`的起始地址是0x000000004017c0，`getbuf`最开始stack分配的空间是40个bytes，将`$rsp`传给`Gets`，因此需要先用40个bytes填充这个stack，然后多出的一部分放`touch1`的地址



#### phase2

需要适量code injection

不仅需要出发`touch2`还需要合适的参数，即val需要是给定cookie字符串

注入的代码需要将stack上的值放入$rdi中

> `mov xxx , $rdi`

需要通过gcc的汇编器将汇编程序`.s`文件转换成二进制可重定位文件`.o`。

在`getbuf`返回的时候，`ret`操作将当前栈的后续8个bytes取出放入`$rip`作为下一个运行的代码，因此可以将`ret`的地址修改成`$rsp+8`处，然后在`$rsp+8`处放mov操作的bytes seq，然后在放`ret`的代码

解决方案：`getbuf`里retq时，`$rsp`本来存的是指向`test`的地址，需要用overflow把这个改成`$rsp+8`的地址（因为ret必须跳转过去运行，因为最终需要我们injected code进行`ret`跳转，而`ret`是看`$rsp`当前存的地址，因此需要人为的`pushq xxx`，不然最终会跳转到`mov`处。然后需要做`mov`操作和`ret`操作。

主要考察了简单的编码、pushq、对`$rsp`的使用



#### phase_3

在`touch3`里还嵌套一层`hexmatch`函数，在`getbuf`跳转到`touch3`时还需要附带一个参数，这个参数需要在`hexmatch`里和cookie做一些比较判断，基本框架和phase_2差不多，只是字符串反推会难一些。

`hexmatch`接受两个参数：第一个是unsigned int，等于cookie，第二个是需要我们自己写的，一个char*。在其内部，在一个大致范围内部生成选择一个地址，将cookie写入，然后将这个地址与我们输入的字符串的地址比较

至于为什么是随机地址，是为了防止在比较的时候取巧，直接两个一样的地址了（误）

解决方案：

从lower bytes开始，先是40个填充字节，然后是一个8字节的地址，跳到当时`$rsp+8`处，然后是一个`pushq和mov`，然后是`ret`。其中`mov`的时候需要把根据cookie编的string地址填上，具体是多少要看`mov和ret`占用多少字节。

objdump后发现占用12个字节（包含pushq），然后编码ret地址是`0x5561dcb3`，因此加一个byte即是`0x5561dcb4`。



####  phase_4

阶段4和5是return-oriented

保护缓冲区溢出：

- 栈随机初始化
- canary
- 限制可执行代码区域

第一种和第三种方法使得code injection非常困难

因为code injection是在stack溢出的区域填充一些可执行代码

而return-oriented programming（ROP)是通过执行stack填充的地址上的代码来完成侵入（是一些pattern片段）



解决方案：

phase_4是完成phase_2同样的工作，即不仅要跳转到touch2的地址，还需要将\$rdi设置成cookie的值。

理想情况是只需要一个符合popq \$rdi 的code pattern，但是找不到

实际上有一个popq \$rax和movq \$rax,\$rdi

因此在buffer溢出的部分分别需要填上第一个pattern的起始地址，cookie数据，第二个pattern的起始地址，touch2的地址

> byte order:地址0x10说明低位是10,高位是0，因为小端的MSB放在higher side

为了保险起见，最好地址的8个byte都进行填充



#### phase_5

不做了







