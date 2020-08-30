# Cache lab

## Part A

在软件层面构造一个缓存模拟器，对一段程序进行定量化的缓存性能描述。

涉及的内容：

- cache memory的结构、替换规则
- getopt、sscanf、fscanf的使用

程序主要框架如下：

- 读取命令行参数
- 初始化缓存
- 在循环体内部不断输入地址

#### getopt

> `include <stdlib.h>`

命令行参数主要支持无参数选项，有参数选项，和可有可无参数选项

`getopt(int argc, char * const argv[], const char *optstring);`

前两个是main函数的参数，第三个是对参数选项的设置，例如有参数`-b`，并且这个是不需要选项的，则在optstring里加一个b，如果必须需要选项，则加`b:`

`getopt`函数返回值正常情况下返回参数的字符，在解析完时会返回-1，解析的结果放在全局变量里

- Optarg:存放选项字符串

#### sscanf、fscanf

在linux环境下，有两套I/O函数，一个是linux的系统调用，更底层，返回的是文件描述符，例如`open`，还有一套是c标准库函数，定义在`<stdio.h>`，例如`fopen`，返回的是流对象FILE*，标准输入输出错误是三个预定义的流，文件流需要手动打开。

对流的操作主要包括读取，写入

- fgets：从流中读取一行到给定的缓冲区
- scanf：从标准输入流读取至缓冲区，格式化
- sscanf：在两个缓冲区之间格式化读取
- fscanf：从文件流中读取，格式化
- fprintf：格式化输出到给定的流
- printf：格式化输出到标准输出



## Part B

这部分任务是矩阵转置，分成三个矩阵，32\*32，64\*64以及

cache的结构为：

- s=5，一共32个set
- E=1，直接映射
- b=5，一个block可以容纳8个int

### 32\*32

在write up里提到可以使用分块的技术，减少miss

每8行矩阵会占满一个cache，尝试使用8\*8的块进行转置

一个8\*8的块中，每一行都在不同的不同的set中

利用8个local variables存放A中块的一行元素，然后依次放入B块中

如果A块在对角线上（一共4个块）,则每次A块的读取（一共8次）都会造成一次miss，一共是8次，B块每行的第一次读取会造成一次miss，同时A块的读取会造成依次eviction，所以一共是16次。因此这四个块造成的miss一共是4*(8+16)=96

如果A块不在对角线上（一共12个块），则每次A块一共造成12次miss，B块每行读取依然是一次miss，但是不会存在conflict miss，因为A块和B块要读取的内容不在一个set里，因此一共是12*(8+8)=196

因此一共是288次miss，实验结果是287，差不多。

### 64\*64

由于一行有64个数字，导致cache只能装4行矩阵，因此如果用4*4的分块+上面的方法，会导致miss大量提升，因为每次读取A的时候miss次数直接翻倍（被载入缓存但是无法利用）

为了降低miss次数，还是得尽可能的把block的所有数据利用好。

因此还是尝试8*8的分块，在读取A的时候先将数据读入B，位置可以不一定对，但之后可以修正过来。



考虑对图中A块进行操作。

- 首先读取A的前四行，每次读取一行，用8个局部变量储存。然后在存入B的时候，假设B2位置是[i,j]，B15位置是[j,i]，正常情况下，会存入B15->B22，但是后四个会造成conflict miss，这样利用率不高，可以先存在缓存中已经有的位置。比如前四个存在B15->B18,后四个存在F15->F18，因为同一行的在同一个cache block里。操作完前四行后，图示如下：

  <img src="./截屏2020-08-30 下午3.17.05.png" alt="截屏2020-08-30 下午3.17.05" style="zoom:25%;" />

  其中B块的前四列数据都是正确摆放的，后四列不对，但是没有miss，还是很好的。

- 接下来需要对A块的后四行进行操作，顺便对之前错误的四列进行修正（因为占用了A块后四行的位置）。注：前四行操作完后，cache里的的四个set是B矩阵的前四行对应的set，同时也是后四行对应的set。然后读取的顺序会有点tricky，先读取B中F15->F18，以及A中的B6->B9，然后分别放入对应的位置，然后操作下一列，一共需要操作四列。如果A不是对角线上的块，则这样操作miss非常低，首先读取B的四个数据时不会有miss，然后读取A的四个元素时，第一次会有miss（cold miss），之后就不会，并且A块和B块这四列的set集合不重叠，因此其实也可以横着读，反正miss都来自于cold miss。图示如下(只操作一次时的截图)

  <img src="./截屏2020-08-30 下午3.35.28.png" alt="截屏2020-08-30 下午3.35.28" style="zoom:25%;" />

- 最后就正常的对右下角的4\*4小块进行转置即可。

miss次数的分析：

- 假如A块在对角线上，则4 \*（1+1+1）+4 \*(4+1)+4+8=44
- 假设A块不在对角线上，则4\*(1+1)+4+4=16
- 一共大约1248次，运行处是1219次，差不多。


