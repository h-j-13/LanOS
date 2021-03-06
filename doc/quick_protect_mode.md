# 真·三十天
## 保护模式速成

### 要讲解的重点

* 什么是boot 什么是header
* 为什么要有保护模式


### 我们的近期目标

上一个视频为大家介绍了二进制文件相关的知识和工具

也介绍了裸机在启动后大概经历的过程

然后演示了一下汇编语言和c语言互相调用的套路

在视频的最后我推荐大家去看一下《源码》第四章及之前的部分

这期我们先设定一个小目标，就是我们要用c语言来改写《源码》第四章的demo

有了这个有一些难度，但是看起来不算太难的小目标，我们在学习的时候就会有方向，不至于在纷杂的代码中迷失

这期的开头我先带大家来看一下《源码》第四章讲了一些什么东西

我们直取要害，先跳过知识点部分，直接看demo效果，然后看代码

之后我们在梳理一下为了达成这个小目标，我们要经历的过程，和要学习的知识点

（演示github链接）

---

（跳到pdf的134页）

这个demo的功能就是手动构建两个符合TSS规范的任务，这个TSS（Task state segment）会在之后详细说明一下
系统不断的切换这两个任务
这两个任务不断做自己的工作，也就是调用系统的功能在屏幕上一个打印A，另一个打印B

大体看一下它的两个汇编文件

分别是boot和head

复习一下上个视频的知识

boot就是512字节的引导扇区

head就是要加载到0地址的代码，所以才叫head，头部

那看一下书中的这两个文件代码风格差异非常大

原因是在编写linux0.12的时候，没有一个能够同时支持16位和32位代码的汇编器

所以linus用as86来编译16位代码，用gas来编译32位代码

而在我们的系统中，统一使用nasm来编译代码，所以汇编的风格是一致的

这里面有一个细节上个视频没有说，就是这512字节是如何把head中的代码折腾到0地址处的

本次我们也好好分析一下这个扇区的功能

那下面我们以自己的代码为例说明

---

(对照 pdf 131页的图来说明)

首先看磁盘布局的图，看makefile

解释一下vfd文件大小的由来（在questions.md 中bios int 0x13)

说明一下为什么不直接挪动到0地址处（bios 中断向量表）

---

### 疑点

jmp dword 8:0 是什么

这里我们先看一下当前的寻址方式

我们知道启动的时候是跑在16位的代码中的

这个16位具体体现在什么方面呢

就是我们的所有寄存器里面都只能装最大2^16这么大的数字

而早期cpu是有20根地址线的，最大能使用的内存是1M，靠一个寄存器是无法完全覆盖的

因为差了4位

所以有段寄存器这种东西

包括 cs ds es gs fs ss

我们用一个段寄存器里面的值和另一个寄存器里面的值组合在一起形成一个地址来访问内存

组合的规则是段寄存器里面的值左移4位再加上另外一个寄存器表示的偏移值

拿boot.s 11行开始的es:bx举例（bios的简单说明在pdf 135页代码18行开始）

再看开头 jmp 7c0h

所以一个地址可以有多种表示方式
比如 0x102c

可以是0:0x102c 0x100:0x2c 0x102:0xc

---

1M这么小的内存访问能力在今天看来实在是太小了，

32位的机器一个寄存器可以表示2^32，所以最多应该能够访问4G的内存

这里2^2=4 2^30=2^10*2^10*2^10=1K*1K*1K=1G

所以是4G

然而在boot.s的前半部分，我们都是使用的ax bx这种16位的寄存器，并没有使用eax ebx这种扩展类型的32位寄存器

这是因为我们还没有打开一个开关，这个开关位于cpu中一个名为cr0寄存器的最低位

如果这个最低位是0，那么现在处于16位模式，而如果我们把它置为1，则就进入了一种高级的模式

我们在这种模式下可以使用32位的寄存器

虽然一个寄存器足以表达4G空间的任何一个地址，但是intel仍然选择使用一个段寄存器和一个存储偏移的寄存器来表示一个地址

比如最后的jmp dword 8:0

至于这个原因我们之后会讨论

做完这条语句的效果是cs寄存器，也就是代码段寄存器变成了8， 而eip指令寄存器变成了0

按照原来的段寄存器和偏移寄存器的解析方式，应该是(0x8<<4)+0=0x80

那么指向的代码地址应该是0x80

但是在这种新的模式下，段寄存器中的数据不是代表一个基地址，而是一个结构体的索引

这个结构体描述的是，一个段的基地址，段的最大长度，还有其他的属性

这相比单一一个寄存器只能保存一个基地址信息有了更多的表达能力

说是一个索引，在哪索引呢，应该是一张表对吧

是的，是一张名为GDT的表，global descriptor table，全局描述符表，而那个索引，被成为段的选择符

全局描述符表，也就是刚才所说的结构，存储着一个段的基地址，段的最大长度，还有其他的属性

我们需要先构建好这张表，把它的基地址和元素个数这两个信息放到cpu的一个叫做gdtr的寄存器中

之后修改cr0的最低位开关，我们新的模式才能生效

这里每个描述符长度有8个字节，表中的第0个描述符默认是不能使用的，必须填写为全0

而第一个描述符的偏移就是8

所以明白了8:0中的8的含义了吧

因为每个描述符的长度都是8字节，所以选择符（索引）的低3位必然是0，这就造成了浪费

当然intel的工程师是绝对不会有这种浪费的，这里会存储一些其他的属性，具体的功能之后会详细讲解

像这种固定长度的索引的低位用来存储属性的精妙做法，之后在内存管理的页表中也会看到

---

那既然我们的描述符结构里面存储的信息更丰富了，会给我们带来什么好处呢

最直观的一个好处就是，硬件有机会判定，一次通过选择符（索引）和偏移值的访问，是否在规定范围之内

这就给了我们一个非常靠谱的硬件检测越界的机制

这个机制使得编写操作系统处理越界的问题非常简单

当有程序由于逻辑bug或者其他一些原因把偏移值计算错误进而访问的时候，硬件会触发一个异常

我们如果提前注册过该异常的处理函数，就能知道哪段代码出错了，并作出合理的善后工作

这里我们看到，硬件实际上是在给我们提供一种保护，所以这种高级模式被称为保护模式

当然保护模式的保护不光体现在越界保护的功能上

之后使用到其他功能的时候我们再讲解

---

我们来简单解析一下8指向的这个描述符的含义

（演示）

---

调试header到main

---

那么到目前为止我们距离第四章的demo的功能还是有差距的

首先，我们只是进入了保护模式，但是代码还是在最高权限运行的，没有退到用户态

那有人会问了，就在最高权限呆着不挺好吗，什么都能干

那就打个比方，最高权限呆着的时候就好像打开了汽车发动机的引擎盖，虽然能完整控制发动机，但是如果没有专业知识和技巧是很危险的

作为一个普通的应用程序开发者，想要的是一个座舱内部的环境

汽车给我提供一个操控面板，给我有限的功能，也能适度的纠正我的错误

跟demo的另外一个差距是，demo中已经构造了两个用户态任务，并且开启了时钟中断

在时钟中断处理函数中，会适时的切换任务，使得任务看起来像是同时在运行

以上所说的两个差距会在接下来的几个demo中慢慢补充

一直到fork_demo

而当到达了fork_demo的时候，我们不但实现了第四章的所有内容，并且任务不是手动构建的，而是通过fork系统调用自动生成的，到这里就超越了第四章的demo

---

那我们简单看一下这个过程中都要实践那些demo

---



