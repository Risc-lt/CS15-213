# Chapter 8 Exceptional Control Flow

PS：关于操作系统的进程等更多讲解可见:

[2022 南京大学 “操作系统：设计与实现” (蒋炎岩)](https://space.bilibili.com/202224425/channel/collectiondetail?sid=192498)

[深入理解计算机系统 | Lecture-14 Exceptions and Processes]( https://www.bilibili.com/video/BV1a54y1k7YE/?p=18&share_source=copy_web&vd_source=ca19f2c81f99784f7bcb7a506ece1946)

### **8.1 异常**

**异常（exception）**是异常控制流的一种形式，一部分由硬件实现（也因此它的具体细节会随系统的不同而有所不同），一部分由操作系统实现。异常位于硬件和操作系统交界的部分。

**注意：**这里的异常和 C++ 或 Java 中的应用级异常是不同的。

异常就是控制流中的突变，用来响应处理器状态中的某些变化。在处理器中，状态被编码为不同的位和信号，状态变化称为**事件（event）**。

![1](Chapter 8/1.png)

任何情况下，当处理器检测到有**事件**发生时，就会通过一张叫做**异常表**的跳转表，进行一个间接过程调用（异常），到**异常处理程序**（操作系统中一个专门用来处理这类事件的子程序）中。

**事件的例子：**发生虚拟内存缺页、算术溢出、一条指令试图除以 0、一个系统定时器产生的信号等。

当异常处理程序完成处理后，根据引起异常的事件的类型，会发生以下三种情况中的一种：

1. 处理程序将控制返回给**当前指令 I(curr)**，即事件发生时正在执行的指令。
2. 处理程序将控制返回给**下一条指令 I(next)**，即如果没有发生异常将会执行的下一条指令。
3. 处理程序**终止被中断的程序。**

#### **8.1.1 异常处理**

系统为每种可能的异常都分配了一个唯一的非负整数的**异常号：**

1. 一部分异常号是处理器设计者分配的（对应硬件部分）。比如被零除、缺页、内存访问违例、断点、算术溢出。
2. 另一部分是由操作系统内核的设计者分配的（对应软件部分）。比如系统调用、来自外部 I/O 设备的信号。

在系统启动时，操作系统分配和初始化一张**异常表**，使得**表目 k 包含异常 k 的处理程序的地址**。

​    ![2](Chapter 8/2.png)

系统在执行某个程序时，处理器检测到发生了一个事件，并确定了对应的异常号 k，就会触发异常。

触发异常：执行间接过程调用，通过异常表的表目 k，转到相应的处理程序。**异常号是到异常表中的索引**，异常表的起始地址放在一个**特殊 CPU 寄存器**——异常表基地址寄存器中。

​    ![3](Chapter 8/3.png)

异常类似过程调用，但有一些不同：

1. 过程调用时，在跳转到处理程序前，处理器将返回地址压入栈中。而在异常中，返回地址是当前指令（事件发生时正在执行的指令）或下一条指令。
2. 处理器也会把一些额外的处理器状态压入栈中，在处理程序返回时，重新开始执行被中断的程序会需要这些状态。。
3. 如果控制从用户程序转移到内核，所有这些项目都被压倒内核栈中，而不是用户栈中。
4. 异常处理程序运行在**内核模式**下，因此它们对所有的系统资源都有完全的访问权限。

一旦硬件触发了异常，剩下的工作就是由异常处理程序在软件中执行。

异常处理结束后，会执行一条特殊的“从中断返回”指令，可选地返回到被中断的程序，该指令将适当的状态弹回到处理器的控制和数据寄存器中，然后将控制返回给别终端的程序。

#### **8.1.2 异常的类别**

异常可以分为 4 类：

1. **中断**：来自 **I/O 设备的信号**；**异步**
2. **陷阱**：有意的异常；同步
3. **故障**：潜在的可恢复的错误；同步
4. **终止**：不可恢复的错误；同步

中断是异步异常，**异步异常**是由处理器**外部的 I/O 设备中的事件**产生的。

陷阱、故障和终止都是同步异常，它们是**执行一条指令（称为故障指令）的直接产物**。因为指令是按时钟周期执行的，所以由指令造成的异常必然是同步的。

**总结：**简单而言，异常可以分为**异步**和**同步**两类，其中异步的异常是来自外部的 I/O 设备，同步的异常则都是指令的产物。

   ![4](Chapter 8/4.png)

**中断**

中断是异步异常，是由处理器外部的 I/O 设备中的事件产生的。硬件中断不是由指令造成的，因此它是异步的。硬件中断的异常处理程序常常叫做**中断处理程序**。

I/O 设备向处理器芯片上的一个引脚发信号，并将异常号放到系统总线上来触发中断。**异常号标识了引起中断的设备。**

在**当前指令完成执行**后，处理器注意到中断引脚的电压变高了，就从系统总线读取异常号，调用对应的中断处理程序。

**陷阱和系统调用**

陷阱是有意的异常，是执行一条指令的结果。

**陷阱最重要的用途**是在应用程序和内核之间提供一个接口，叫做**系统调用**。

系统提供了一条特殊的 ”syscall n“ 指令，当用户程序想要向内核请求服务 n 时，就执行这条指令。执行 syscall 指令会导致一个到异常处理程序的陷阱（异常），这个处理程序解析参数，并调用适当的内核程序。

**用户程序经常需要向内核请求服务**，比如读文件（read）、创建进程（fork）、加载程序（execve）、终止进程（exit）等。

从程序员角度看，系统调用和普通的函数调用是一样的。但是实现上大不相同。

**故障**

**故障由错误情况引起。**故障发生时，处理器将控制转移给故障处理程序，如果处理程序能够修正错误，就把控制返回到引起故障的指令，否则返回给内核中的 abort 例程，abort 会终止当前的应用程序。

**缺页异常**

**缺页异常**是一种经典的故障（页面是虚拟内存中一个连续的块，典型值是 4KB）。当指令引用一个虚拟地址，而**与该地址对应的物理页面不在内存中，必须要从磁盘取出时**，就会发生缺页异常。

然后**缺页处理程序会从磁盘加载适当的页面**，然后将控制返回给引起故障的指令。**当指令再次执行时，相应的物理页面就在内存中了。**

理解：从存储器层次结构的角度看，缺页异常似乎可以看作是内存不命中的惩罚。

**终止**

终止是**不可恢复的致命错误**造成的结果，通常是一些硬件错误。终止处理程序将控制返回给一个 **abort** 例程，该例程会终止这个应用程序。

**理解：运行程序时遇到了 abort 表明发生了故障或终止异常。**

#### **8.1.3  Linux/x86-64系统中的异常**

x86-64 系统中有 256 种不同的异常类型，其中 0~31 号是 Intel 架构师定义的异常，剩下的是操作系统定义的**中断**和**陷阱**。

![5](Chapter 8/5.png)

理解：**0-31 号是故障或终止**，**32~255 号都是操作系统定义的中断或系统调用。**

​    ![6](Chapter 8/6.png)

**Linux/x86-64 故障和终止**

1. **除法错误。**当应用试图除以零时，或者当一个除法指令的结果对目标操作数来说太大了，就会发生除法错误。
   1. 当发生除法错误，Unix 会**直接终止程序**，Linux shell 通常把除法错误报告为**“浮点异常(Floating Exception)”**。
2. **一般保护故障。**有许多原因，通常是因为一个程序引用了一个未定义的虚拟内存区域，或试图写一个只读的文本段。
   1. 此类故障也不会恢复，Linux shell 通常会把一般保护故障报告为**“段故障(Segmentation fault)”**。
3. **缺页异常。**此类故障会尝试恢复并重新执行产生故障的指令。
4. **机器检查。**在告知故障的指令执行中检测到致命的硬件错误时发生。
   1. 此类故障从不返回控制给应用程序。

**Linux/x86-64 系统调用**

Linux 提供几百种系统调用，供应用程序请求内核服务时使用。（其中有一部分在 unistd.h 文件中）

系统中有一个跳转表（类似异常表）。**每个系统调用都有一个唯一的整数号**，对应一个到内核中跳转表的偏移量。

C 程序使用 **syscall 函数**可以直接调用任何系统调用。但是没必要这么做，C 标准库为大多数系统调用提供了包装函数。这些包装函数也是**系统级函数**。

所有 Linux 系统调用的参数都是通用寄存器而不是栈传递的。一般寄存器 **%rax** 包含系统调用号。

系统调用是通过陷阱指令 syscall 进行的（这里的 syscall 指令是汇编级别的指令）。 

​    ![7](Chapter 8/7.png)

​                '系统级函数写 helloworld' int main() {    write(1, "hello, world\n", 13);    _exit(0); } '汇编语言版本（省略了绝大部分代码）' main:    movq $1, %rax    syscall              

### **8.2 进程**

**异常**是允许操作系统内核提供进程概念的基本构造块。

进程的经典定义就是**一个执行中程序的实例**。系统中的**每个程序都运行在某个进程的上下文（context）中**。

**上下文**是由程序正确运行所需的状态组成的。这个状态包括存放在内存中的程序的代码和数据，它的栈、通用目的寄存器的内容、程序计数器、环境变量、打开文件描述符的集合。

当用户向 shell 输入一个可执行目标文件的名字，运行程序时，**shell 就会创建一个新的进程，然后在这个新进程的上下文中运行该可执行文件**。

进程提供给应用程序的关键抽象：

1. 一个独立的逻辑控制流。好像程序独占地使用处理器。
2. 一个私有的地址空间。好像程序独占地使用内存。

#### **8.2.1 逻辑控制流**

使用调试器单步执行程序时会看到一系列的程序计数器（PC）值，这个 PC 的值的序列叫做**逻辑控制流**，简称**逻辑流**。

PC 的值唯一地对应于包含在程序的**可执行目标文件中的指令**，或包含**在运行时动态链接到程序的共享对象中的指令**。

逻辑流有许多不同的形式，异常处理程序、进程、信号处理程序、线程等都是逻辑流的例子。

​    ![8](Chapter 8/8.png)

进程是轮流使用处理器的。

#### **8.2.2 并发流**

当一个逻辑流的执行在时间上与另一个流重叠，就称为**并发流**，这两个流称为并发地运行。

**并行流**是并发流的**真子集**，如果两个流并发地运行在不同的处理器核或不同的计算机上时，就称为并行流。

一个进程和其他进程轮流运行的概念叫做**多任务**。

一个进程执行它的控制流的一部分的每一时间段叫做**时间片**。如 8.2.1 的图中的进程 A 包含两个时间片。

#### **8.2.3 私有地址空间**

进程为每个程序提供它自己的私有地址空间。一般而言，和这个私有地址空间中某个地址相关联的那个内存字节是不能被其他进程读或写的。

不同进程的私有地址空间关联的内存的内容一般不同，但是每个这样的空间都有相同的通用结构。

**x86-64 Linux 进程的地址空间的组织结构**

地址空间的**顶部保留给内核（操作系统常驻内存的部分）**，包含内核在代表进程执行指令时（比如当执行了系统调用时）使用的代码、数据、堆和栈。

地址空间的底部留给用户程序，包括代码段、数据段、运行时堆、用户栈、共享库等。代码段总是从地址 0x400000 开始。

**理解：可以看出，内核栈和用户栈是分开的。**

![9](Chapter 8/9.png)

#### **8.2.4 用户模式和内核模式**

处理器使用**某个控制寄存器中的一个模式位（mode bit）**来区分**用户模式**与**内核模式**。进程初始时运行在用户模式，当设置了模式位时，进程就运行在内核模式。

- 运行在内核模式的进程可以执行指令集中的**任何指令**，并可以访问系统中的**任何内存位置**。
- 运行在用户模式的进程**不允许执行特权指令**，比如停止处理器、**改变模式位、发起 I/O 操作**等，也不能直接引用地址空间内核区中的代码和数据，用户程序**只能通过系统调用接口间接地访问内核代码和数据**。

进程**从用户模式变为内核模式的方法**是通过中断、故障、陷阱（系统调用就是陷阱）这样的异常。异常发生时，控制传递给异常处理程序，处理器将模式从用户模式转变为内核模式。

**/proc 文件系统**

Linux 提供了一种叫做 **/proc 文件系统**的机制来允许用户模式进程访问内核数据结构的内容。

/proc 文件系统将许多内核数据结构的内容输出为一个用户程序可以读的文本文件的层次结构。

可以通过 /proc 文件系统找出**一般的系统属性**（如 CPU 类型：/proc/cpuinfo）或者**某个特殊的进程使用的内存段**（/proc//maps）。

2.6 版本的 Linux 内核引入了 **/sys 文件系统**，它输出关于系统总线和设备的额外的低层信息。

#### **8.2.5 上下文切换**

**上下文切换**是一种**较高层形式的异常控制流**，它是建立在中断、故障等较低层异常机制之上的。

**系统通过上下文切换来实现多任务。**

内核为每个进程维持一个上下文，**上下文是内核重新启动一个被挂起的进程所需的状态。**

上下文由一些对象的值（是这些对象的值而非对象本身）组成，这些对象包括：通用目的寄存器、浮点寄存器、状态寄存器、程序计数器、用户栈、内核栈和各种内核数据结构（如描述地址空间的**页表**、包含有关当前进程信息的**进程表**、包含进程已打开文件的信息的**文件表**）。

内核挂起当前进程，并重新开始一个之前被挂起的进程的决策叫做**调度**，是由内核中的调度器完成的。

内核使用上下文切换来调度进程：

1. 保存当前进程的上下文
2. 恢复某个先前被抢占的进程被保存的上下文
3. 将控制传递给这个新恢复的进程

当内核代表用户执行系统调用时，可能发生上下文切换。如果系统调用因为等待某个事件而阻塞（比如 sleep 系统调用显式地请求让调用进程休眠，或者一个 read 系统调用要从磁盘度数据），内核就可以让当前进程休眠，切换到另一个进程。即使系统调用没有阻塞，内核也可以进行上下文切换，而不是将控制返回给调用进程。

中断也可能引发上下文切换。如所有的系统都有一种定时器中断机制，即产生周期性定时器中断，通常为 1ms 或 10ms。当发生定时器中断，内核就判定当前进程已经运行了足够长时间，该切换到新的进程了。

​    ![10](Chapter 8/10.png)

### **8.3 系统调用错误处理**

当 **Unix 系统级函数**遇到错误时，它们通常会**返回 -1**，并设置**全局整数变量 errno** 来表示什么出错了。

程序员应该总是检查错误

strerror 函数返回一个文本串，描述了和某个 errno 值相关联的错误。**使用 strerror 来查看错误**

```    c
// 调用 Unix fork 时检查错误
if((pid = fork()) < 0) //如果发生错误，此时 errno 已经被设置为对应值了
{
    fprintf(stderr, "fork error: %s\n", strerror(errno));//strerror(errno) 返回描述当前 errno 值的文本串
    exit(0);
}              
```

**错误处理包装函数**

许多人因为错误检查会使代码臃肿、难读而放弃检查错误。可以通过**定义错误报告函数及对原函数进行包装**来简化代码。

对于一个给定的基本函数，定义一个首字母大写的包装函数来检查错误。

```                c
// 错误报告函数
void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
// fork 函数的错误处理包装函数 Fork
pid_t Fork(void)
{
    pid_t pid;
    if ((pid = fork()) < 0)
        unix_error("Fork error"); //调用上面定义的包装函数
    return pid;    
}
```

### **8.4 进程控制**

#### **8.4.1 获取进程ID**

每个进程都有一个唯一的非零正整数表示的进程 ID，叫做 **PID**。有**两个获取进程 ID 的函数**：

1. **getpid 函数：**返回调用进程的 PID（类型为 **pid_t**，在 type.h 中定义了 pid_t 为 int）。
2. **getppid 函数：**返回它的父进程的 PID。

```             c
#include<sys/types.h>
#include<unistd.h>

pid getpid(void);
pid getppid(void);
```

#### **8.4.2 创建和终止进程**

进程总是处于以下三种状态之一：

1. **运行**。进程要么在 CPU 上执行，要么在等待被执行且最终被内核调度。
2. **停止**。进程的执行被挂起且不会被调度。当收到 SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU 信号时，进程就会停止，直到收到一个 **SIGCONT** 信号时再次开始运行。
3. **终止**。进程永远地停止了。进程有三种原因终止：**收到一个信号，该信号的默认行为是终止进程**；**从主进程返回**；**调用 exit 函数**。

**信号**是一种软件中断的形式。

**终止进程**

```    c
#include <stdlib.h>
void exit(int status); // status 指定进程终止时的退出状态。
```

exit 函数以 **status 退出状态**来终止进程（另一种设置退出状态的方法是**从主程序中返回一个整数值**。理解：是否指 main 函数的返回值）。

**创建进程**

父进程通过调用 **fork 函数**创建一个新的运行的子进程。

fork 函数**只被调用一次，但是会返回两次**：一次返回是在父进程中，一次是在新创建的子进程中。**父进程中返回子进程的 PID，子进程中返回 0。**

因为 fork 创建的子进程的 PID 总是非零的，所以可以根据返回值是否为 0 来分辨是当前是在父进程还是在子进程。

``````c
#include <sys/types.h>
#include <unistd.h>
pid_t fork(void); //子进程返回 0，父进程返回子进程的 PID，如果出错，返回 -1
``````

子进程与父进程几乎完全相同：

- 子进程得到与父进程**用户级虚拟地址空间**相同但独立的一份副本，包括**代码段、数据段、堆、共享库、用户栈**。
- 子进程获得与父进程**所有文件描述符相同的副本**。这意味着**子进程可以读写父进程打开的任何文件**。

子进程和父进程之间的**最大区别在于 PID 不同**。

**一个例子**

```   c
int main()
{
	pid_t pid;
	int x = 1;

	pid = Fork();
	if(pid == 0) {
		printf("child : x=%d, id = %d\n", ++x, getpid());
		exit(0);
	}

	printf("parent: x=%d, id = %d\n", --x, getpid());
	exit(0);
}
```

上面程序的执行结果是：

```               c
parent: x=0, id = 3087
child : x=2, id = 3088
```

理解上面例子的一些细节：

1. **调用一次，返回两次。**fork 函数被父进程调用一次，但是返回两次：一次是返回到父进程，一次是返回到子进程。（fork 调用前只有父进程，调用后多了子进程，因此在两个进程里都要返回）
2. 不论子进程还是父进程，它的返回点都是调用 fork 函数的地方，因此在 fork 函数调用之前的部分只会在父进程中执行，而不会在子进程中执行。
3. **并发执行。**父进程和子进程是并发执行的独立进程，内核会交替执行它们的逻辑控制流中的指令。
4. 相同但是独立的地址空间。两个进程的地址空间的内容是完全相同的，但是是互相独立的。
5. 共享文件。

![11](Chapter 8/11.png)

**另一个例子**

```               c
int main()
{
    Fork();
    Fork();
    printf("hello\n");
    exit(0);
}
```

上面调用了两次 Fork，总共产生了四个进程。

​    ![12](Chapter 8/12.png)

#### **8.4.3 回收子进程**

当一个进程终止时，内核并不会立即把它删除。相反，进程被保持在一种已终止的状态中，直到被它的父进程回收。

当父进程回收已终止的子进程时，内核将子进程的退出状态传递给父进程，然后清除子进程。

**僵死进程**：一个终止了但还未被回收的进程。

**init 进程**

系统启动时内核会创建一个 **init 进程，它的 PID 为 1，不会终止，是所有进程的祖先**。

如果一个父进程终止了，init 进程会成为它的孤儿进程的养父。**init 进程会负责回收没有父进程的僵死子进程**。

长时间没有运行的程序，总是应该回收僵死子进程。即使僵死子进程没有运行，也在消耗系统的内存资源。

**waitpid 函数**

一个进程可以通过调用 **waitpid 函数**来等待它的子进程终止或停止。

```c
#include <sys/types.h>
#include <sys/wait.h>
/* 	
	如果成功，返回对应的已终止的子进程的 PID；如果其他错误，返回 -1
	只有当参数 options=WNOHANG 时，才有可能返回 0；其他情况要么返回子进程 PID，要么返回 -1
*/
pid_t waitpid(pid_t pid, int *statusp, int options);  
```

waitpit 函数比较复杂。默认情况下 **options = 0**，此时 waitpid 会挂起调用进程的执行，直到它的**等待集合**中的一个子进程终止。如果等待集合中的一个进程在刚调用时就已经终止了，那么 waitpid 就立即返回。waitpid 的返回值是**对应的已终止的子进程的 PID**，此时该子进程会被回收，内核从系统中删除掉它的所有痕迹。

**1 判定等待集合的成员**

等待集合的成员是由参数 pid 确定的：

- 如果 **pid>0**，等待集合就是**进程 ID=pid** 的那一个特定的子进程。
- 如果 **pid=-1**，等待集合就是由父进程的所有子进程共同构成的。
- 还有其他等待集合，不再讨论。

**2 修改默认行为**

默认情况下 options=0，可以将 options 设置为常量 WNOHANG, WUNTRACED, WCONTINUED 的各种组合来修改默认行为：

- options=0，挂起调用进程的执行，直到它的等待集合中的一个子进程终止。如果等待集合中的一个进程在刚调用时就已经终止了，那么 waitpid 就立即返回。
- options=WNOHANG，如果等待集合中的一个子进程终止了，返回该子进程 ID，如果没有子进程终止，也立即返回，返回值为 0。**WNOHANG 的特点是立即返回，不会挂起调用进程。**
- options=WUNTRACED，挂起调用进程的执行，直到等待集合中的一个进程变成已终止或被停止，返回值是该子进程 ID。WUNTRACED 的特点是还可以检查被停止的子进程。
- options=WCONTINUED，挂起调用进程的执行，直到等待集合中一个正在运行的进程终止或等待集合中一个被停止的进程收到 SIGCONT 信号重新开始执行。
- options=WNOHANG|WUNTRACED，立即返回，如果等待集合中的子进程都没有被停止或终止，则返回 0，否则返回该子进程的 PID。

**3 检查已回收子进程的退出状态**

如果 statusp 参数是非空的，那么 waitpid 就会在 status 中放上关于导致 waitpid 返回的子进程的状态信息，status 是 statusp 指向的值。

**wait.h** 头文件定义了解释 status 参数的几个宏：

- **WIFEXITED(status)**：如果子进程通过调用 **exit 或者一个返回（return）**正常终止，就返回真。
- **WEXITSTATUS(status)**：返回一个正常终止的子进程的退出状态。只有在 WIFEXITED() 返回为真时，才会定义这个状态。
- WIFSIGNALED(status)：如果子进程是因为一个未被捕获的信号终止的，那么就返回真。
- WTERMSIG(status)：返回导致子进程终止的信号的编号。只有在 WIFSIGNALED() 返回为真时，才定义这个状态。
- **WIFSTOPPED(status)**：如果引起返回的子进程当前是停止的，就返回真。
- WSTOPSIG(status)：返回引起子进程停止的是信号的编号。只有在 WIFSTOPPED() 返回为真时，才定义这个状态。
- WIFCONTINUED(status)：如果子进程收到 SIGCONT 信号重新启动，则返回真。

**4 错误条件**

如果调用进程没有子进程，那么 waitpid **返回 -1**，并设置 errno 为 **ECHILD**。如果 waitpid 函数被一个信号中断，那么它**返回 -1**，并设置 errno 为 **EINTR**。

**5 wait函数**

**wait 函数**是 waitpid 函数的简单版本。

```c
#include <sys/types.h>
#include <sys/wait.h>
pid_t wait(int *statusp);  //如果成功，返回子进程的 PID，如果出错，返回 -1
```

**6 使用waitpid的示例**

```c
#include "csapp.h"
#define N 2

int main()
{
	int status, i;
	pid_t pid;

	/* Parent creates N children */
	for (i=0; i<N; i++)
		if((pid = Fork()) ==0)
			exit(100+i);

	/* parent reaps N children in no particular order */
	while((pid = waitpid(-1, &status, 0))>0){
		if(WIFEXITED(status))
			printf("child %d terminated normally with exit status=%d\n", pid, WEXITSTATUS(status));
		else
			printf("child %d terminated abnormally\n", pid);
	}

	/* The only normal termination is if there are no more children */
	if(errno != ECHILD)
		unix_error("waitpid error");
	exit(0);
}
```

#### **8.4.4 让进程休眠**

**sleep函数**

**sleep 函数**将一个进程挂起一段指定的时间。注意：sleep 不是 C 标准库里的函数，是 unistd 中的控制进程的函数。

```c
#include <unistd.h>
unsigned int sleep(unsigned int secs); //返回还要休眠的秒数
```

如果请求的休眠时间量到了，sleep 返回 **0**，否则返回**还剩下的要休眠的秒数**（当 sleep 函数被一个信号中断而过早地返回，会发生这种情况）。

**pause函数**

pause 函数让调用函数休眠，**直到该进程收到一个信号**。

```c
#include <unistd.h>
int pause(void);
```

#### **8.4.5 加载并运行程序**

**execve函数**

execve 函数在当前进程的上下文中加载并运行一个新程序（是程序不是进程）。

             ```        c
             #include <unistd.h>
             int execve(const char *filename, const char *argv[], const char *envp[]); 
             //如果成功，则不返回，如果错误，返回 -1。
             ```

execve 函数功能：加载并运行**可执行目标文件 filename**，并带**一个参数列表 argv** 和**一个环境变量列表 envp**。

execve **调用一次并从不返回**（区别于 fork 调用一次返回两次）。

参数列表和变量列表：

1. **参数列表**：argv 指向一个以 null 结尾的指针数组，其中每个指针指向一个字符串。
2. **环境变量列表**：envp 指向一个以 null 结尾的指针数组，其中每个指针指向一个环境变量字符串，每个串都是形如 “name=value” 的名字-值对。

**execve函数的执行过程**

execve 函数调用加载器加载了 filename 后，设置用户栈，并将控制传递给新程序的主函数（即 main 函数）。

**main 函数**

main 函数有以下形式的原型，两种是等价的。

               ```c
               int main(int argc, char **argv, char **envp);
               int main(int argc, char *argv[], char *envp[]);
               ```

main 函数有三个参数：

1. **argc**：给出 argv[ ] 数组中非空指针的数量。
2. **argv**：指向 argv[ ] 数组中的第一个条目。
3. **envp**：指向 envp[ ] 数组中的第一个条目。

argc 和 argv 的值都是从命令行中获取的，如果命令行中只有该可执行文件的名字，没有其他参数，则 argc=1，argv 的第一个元素的值即为该可执行文件的文件名（包含路径）

注意 argv[] 数组和 envp 数组最后一个元素都是 **NULL**，可以使用 NULL 作为循环终止条件来遍历数组。

​    ![13](Chapter 8/13.png)

**操作环境变量数组的函数**

             ```c
             #include <stdlib.h>
             //在环境变量列表中搜索字符串 "name=value"，如果搜到了返回指向 value 的指针，否则返回 NULL
             char *getenv(const char *name);  
             /*
             	如果环境变量列表中包含一个形如 ”name=value" 的字符串，setnv 会用 newvalue 替代原来的 value，
             	如果不存在，直接添加一个 "name=newvalue" 到数组中。
             */
             int setenv(const char *name, const char *newvalue, int overwrite);//若成功返回 0，否则返回 -1。
             //如果环境变量列表中包含一个形如 ”name=value" 的字符串,unsetnv 会删除它。
             void unsetenv(const char *name); 
             ```

**区分程序与进程**

**程序**：程序是一堆代码和数据，程序可以作为目标文件存在于磁盘上，或作为段存在于虚拟地址空间中。

**进程：进程是执行中程序的一个具体的实例**。

程序总是运行在某个进程的上下文中。

**区分 fork 和 execve**

fork 函数是在新的子进程中运行相同的程序，新的子进程是父进程的一个复制品。

execve 函数是在当前进程的上下文中加载并运行一个新的程序，它会覆盖当前进程的地址空间，但没有创建一个新的进程。

#### **8.4.6 利用fork和execve运行程序**

像 Unix shell 和 Web 服务器这样程序大量使用了 fork 和 execve 函数

**一个简单的 shell 的实现方式**

shell 会打印一个命令行提示符，等待用户在 stdin 上输入命令行，然后对这个命令行求值。

**shell 的 main 例程**

               ```c
               #include "csapp.h"
               #define MAXARGS   128
               
               int main(){
                   char cmdline[MAXLINE]; /* Command line */
                   while (1) {
                       /* Read */
                       printf("> ");
                       Fgets(cmdline, MAXLINE, stdin);  //读取用户的输入
                       if (feof(stdin))
                           exit(0);
               
                       /* Evaluate */
                       eval(cmdline);  //解析命令行
                   }
               }
               ```

**解释并执行一个命令行**

```        c
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);  //调用 parseline 函数解析以空格分隔的命令行参数
    if (argv[0] == NULL)   //表示是空命令行
        return; /* Ignore empty lines */

    //调用 builtin_command 检查第一个命令行参数是否是一个内置的 shell 命令。如果是的话返回 1，并在函数内就解释并执行该命令。
    if (!builtin_command(argv)) //如果返回 0，即表明不是内置的 shell 命令
    {
        if ((pid = Fork()) == 0)    //创建一个子进程
        { /* Child runs user job */
            if (execve(argv[0], argv, environ) < 0)    //在子进程中执行所请求的程序
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        /* Parent waits for foreground job to terminate */
        if (!bg) // bg=0 表示是要在前台执行的程序，shell 会等待程序执行完毕
        {
            int status;
            if (waitpid(pid, &status, 0) < 0)  //等待子进程结束回收该进程
                unix_error("waitfg: waitpid error");
        }
        else   // bg=1 表示是要在后台执行的程序，shell 不会等待它执行完毕
            printf("%d %s", pid, cmdline);
    }
    return;
}
```

注意：上面的 shell 程序有缺陷，它只回收了前台的子进程，没有回收后台子进程。

还有一个 **parseline 函数**和 **builtn_command 函数**不再列出。 其中 parseline 函数负责**解析以空格分隔的命令行参数字符串**并**构造**要传递给 evecve 的 **argv 向量**。builtn_command 函数负责检查**第一个命令行参数是否是一个内置的 shell 命令**。             

### **8.5 信号**

**信号**是一种更高层次的**软件形式的异常**，它**允许进程和内核中断其他进程**。

一个信号就是一条消息，它通知进程系统中发生了某件事情。

每种信号类型都对应于某种系统事件。信号可以简单分为两类：

1. 一类信号对应低层的硬件异常，此类异常由内核异常处理程序处理，对用户进程不可见。但是当发生异常，会通过信号通知用户进程发生了异常。
   1. 例子：一个进程试图除以 0，内核会发送给他一个 SIGFPE 信号。
2. 一类信号对应于内核或其他用户进程中叫高层的软件事件。
   1. 例子：用户按下 **Ctrl+C**，内核会发送一个 **SIGINT** 信号给这个前台进程组中的每个进程。一个进程可以通过向另一个进程发送 **SIGKILL** 信号强制终止它。

​    ![14](Chapter 8/14.png)

上面列出的这些信号是一些宏，类型是 int，值就是它们的编号。

#### **8.5.1 信号术语**

传送一个信号到目的进程包含两个步骤：

1. **发送信号。**内核通过**更新目的进程上下文中的某个状态**，发送一个信号给目的进程。
2. **接收信号**。当目的进程被内核强迫**以某种方式对信号的发送做出反应**时，就接受了信号。进程可以**忽略、终止**或通过执行一个叫做信号处理程序的用户层函数**捕获**这个信号。

发送信号有两种原因：

1. 内核检测到一个系统事件，比如除零错误或子进程终止。
2. 一个进程调用了 **kill** 函数，显式地要求内核发送一个信号给目的进程。一个进程可以发送信号给自己。

​    ![15](Chapter 8/15.png)

**待处理信号**

**待处理信号**是指发出但没有被接收的信号。

**一种类型最多只会有一个待处理信号**。如果一个进程已经有一个类型为 k 的待处理信号，接下来发给他的类型为 k 的信号都会直接丢弃。

进程可以**有选择地阻塞接收某种信号**。当一种信号被阻塞，它仍可以发送，但是产生的待处理信号不会被接收。

一个待处理信号最多只能被接收一次。内核为每个进程在 **pending 位向量**中维护着待处理信号的集合，在 **blocked 位向量**中维护着被阻塞的信号集合。

只要传送了一个类型为 k 的信号，内核就会设置 pending 中的第 k 位，只要接收了一个类型为 k 的信号，内核就会清除 blocked 中的第 k 位。

#### 8.5.2 发送信号

Unix 系统提供了大量向进程发送信号的机制。这些机制都是基于**进程组（process group）**的概念。

**进程组**

每个进程都只属于一个进程组，进程组由一个正整数进程组 ID 来标识。

可以使用 **getpgrp 函数**获取当前进程的进程组 iD，可以使用 **setpgid** 函数改变自己或其他进程的进程组。

默认情况下，子进程和它的父进程同属于一个进程组。

```                   c
#include<unistd.h>
pid_t getpgrp(void);   // 返回调用进程的进程组 ID
/*
	将进程 pid 的进程组改为 pgid。若成功返回 0，错误返回 -1。
    如果 pid=0，就表示使用当前进程的 pid，如果 pgid=0，就表示要将使用第一个参数 pid 作为进程组 ID。 
*/
int setpgid(pid_t pid, pid_t pgid);     
```

**用/bin/kill程序发送信号**

可以用 Linux 中的 **kill 程序**向另外的进程发送任意的信号。

**正的 PID** 表示发送到对应进程，**负的 PID** 表示发送到对应进程组中的每个进程。

```       bash
linux> /bin/kill -9 15213      # 发送信号9（SIGKILL）给进程 15213。
linux> /bin/kill -9 -15213     # 发送信号9（SIGKILL）给进程组 15213 中的每个进程。
```

上面使用了完整路径，因为有些 Unix shell 有自己内置的 kill 命令。

**从键盘发送信号**

Unix shell 使用**作业**这个抽象概念来表示为对一条命令行求值而创建的进程。在任何时刻，至多只有一个前台作业，后台作业可能有多个。

```     bash
linux> ls | sort  # 这会创建两个进程组成的前台作业，这两个进程通过 Unix 管道连接起来：一个进程运行 ls 程序，另一个运行 sort 程序
```

shell 为每个作业创建一个独立的进程组，进程组 ID 通常取作业中父进程中的一个。

​    ![16](Chapter 8/16.png)

上图是一个包含一个前台作业与两个后台作业的 shell。

在键盘上输入 **Ctrl+C** 会导致内核发送一个 **SIGINT 信号**到前台进程组中的每个进程，默认情况下会终止前台作业。

输入 **Ctrl+Z** 会发送一个 **SIGTSTP 信号**到前台进程组中的每个进程，默认情况下会**停止（挂起）前台作业**。

**用kill函数发送信号**

进程可以通过调用 kill 函数发送信号给其他进程（包括自己）。

              ```           c
              #include<sys/types.h>
              #include<signal.h>
              int kill(pid_t pid, int sig);  //若成功则返回 0，若错误则返回 -1
              ```

pid 取值有三种情况：

1. pid>0：kill 函数发送信号号码 sig 给进程 pid。
2. pid=0：kill 函数发送信号 sig 给调用进程所在进程组中的每个进程，
3. pid<0：kill 函数发送信号 sig 给进程组 | pid | (pid 的绝对值)中的每个进程。 

**用alarm函数发送信号**

进程可以通过调用 **alarm 函数**向他自己发送 SIGALRM 信号。

``` c
#include<unistd.h>
unsigned int alarm(unsigned int secs);  
//返回前一次闹钟剩余的描述，如果以前没有设定闹钟，就返回 0。
```

alarm 函数安排内核在 **secs 秒后**发送一个 **SIGALRM 信号**给调用进程。如果 secs=0，则不会调度安排新的闹钟。

在任何情况下，对 alarm 的调用都将取消任何待处理的闹钟，并返回任何待处理的闹钟在被发送前还剩下的秒数。如果没有待处理的闹钟，就返回 0。

**发送信号的方法总结**

1. 内核给进程/进程组发送信号。
2. 使用 /bin/kill 程序给进程/进程组发送任意信号。
3. 调用 kill 函数给进程/进程组发送任意信号。
4. 进程调用 alarm 函数给自己发送 SIGALRM 信号。
5. 键盘按键来发送信号。

#### **8.5.3 接收信号**

当内核把进程 p **从内核模式切换到用户模式时**（比如从系统调用返回），他会检查进程 p 的未被阻塞的**待处理信号的集合**。如果集合为空，内核就将控制传递到 p 的逻辑控制流中的下一条指令；如果集合非空，内核就选择集合中的某个信号（通常是最小的 k），并强制 p 接收信号 k。

进程 p 收到信号会触发 p 采取某种行为，等进程完成了这个行为，控制就传递回 p 的逻辑控制流中的下一条指令。

每个信号类型都有一个预定义的默认行为，是下面中的一种：

1. 进程终止。
2. 进程终止并转储内存。
3. 进程停止（被挂起）直到被 **SIGCONT 信号**重启。
4. 进程忽略该信号。

进程可以通过 **signal 函数**修改和信号相关联的默认行为，其中 SIGSTOP 和 SIGKILL 的默认行为不能修改。

signal 是在 C 标准库的头文件 signal.h 中定义的。

```             c
#include<signal.h>  
typedef void (*sighandler_t)(int); 
// sighandler_t是一个类型别名，代表了一个指向参数为整数且返回类型为void的函数的指针
sighandler_t signal(int signum, sighandler_t handler); 
// 若成功返回指向前次处理程序的指针，若出错则返回 SIG_ERR（不设置 errno）。
```

signal 函数接受两个参数：信号值和函数指针，可以通过下列三种方法之一来改变和信号 signum 相关联的行为：

1. 如果 handler 是 SIG_IGN，那么忽略类型为 signum 的信号。SIG_IGN 是 signal.h 中定义的一个宏。
2. 如果 handler 是 SIG_DFL，那么类型为 signum 的信号行为恢复为默认行为。SIG_DEF 是 signal.h 中定义的一个宏。
3. 如果 hanlder 是用户定义的函数的地址，这个函数就被称为信号处理程序。只要进程接收到一个类型为 signum 的信号，就会调用这个程序。通过把处理程序的地址传递到 signal 函数来改变默认行为，这叫做设置信号处理程序。调用信号处理程序被称作捕获信号。执行信号处理程序被称作处理信号。

当处理程序执行它的 return 语句时，控制传递回控制流中进程被信号接收中断位置处的指令。

信号处理程序可以被其他信号处理程序中断。

​    ![17](Chapter 8/17.png)

**一个信号处理程序的例子**

```c
#include "csapp"
void sigint_handler(int sig)  //定义了一个信号处理程序
{
    printf("Caught SIGINT!\n");
    exit(0);
}
int main()
{
    /* Install the SIGINT handler */
    if(signal(SIGINT, sigint_handler)) == SIGERR)
        unix_error("signal error");
    pause(); // wait for the receipt of signal
    return 0;    
}
```

#### **8.5.4 阻塞和解除阻塞信号**

Linux 提供两种阻塞信号的机制：

1. 隐式阻塞机制。内核默认阻塞任何当前处理程序正在处理信号类型的待处理的信号。
2. 显式阻塞机制。应用程序可以使用 **sigprocmask 函数和它的辅助函数**，明确地阻塞和解除阻塞选定的信号。

**sigprocmask函数**

sigprocmask 函数改变当前阻塞的信号集合（blocked 位向量），具体行为依赖 how 的值：

1. SIG_BLOCK：把 set 中的信号添加到 blocked 中（blocked = blocked | set）。
2. SIG_UNBLOCK：从 blocked 中删除 set 中的信号（blocked = blocked & ~set)。
3. SIG_SETMASK：block = set。

             ```             c
             #include<signal.h>
             int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);  
             ```

如果 oldset 非空，blocked 位向量之前的值保存在 oldset 中。

**其他辅助函数**

辅助函数用来对 set 信号集合进行操作：

1. sigemptyset 初始化 set 为空集合；
2. sigfillset 把每个信号都添加到 set 中；
3. sigaddset 把信号 signum 添加到 set 中；
4. sigdelset 把信号 signum 从 set 中删除。如果 signum 是 set 的成员返回 1，不是返回 0。

```           c
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(segset_t *set, int signum);
```

**一个临时阻塞 SIGINT 信号的例子**

```c
sigset_t mask, prev_mask;
Sigemptyset(&mask);   
Sigaddset(&mask, SIGINT);  //将 SIGINT 信号添加到 set 集合中

Sigprocmask(SIG_BLOCK, &mask, &prev_mask);  //阻塞 SIGINT 信号，并把之前的阻塞集合保存到 prev_mask 中。
...                                         //这部分的代码不会被 SIGINT 信号所中断
Sigprocmask(SIG_SETMASK, &prev_mask, NULL); //恢复之前的阻塞信号，取消对 SIGINT 的阻塞
```

#### **8.5.5 编写信号处理程序**

信号处理是 Linux 系统编程最棘手的问题。

处理程序的几个复杂属性：

1. 处理程序与主程序和其他信号处理程序并发运行，共享同样的全局变量，可能和主程序与其他处理程序互相干扰。
2. 如何接收信号及何时接收信号的规则常常有违人的直觉。
3. 不同的系统有不同的信号处理语义。

**一个信号处理程序的例子**

信号处理程序应该接收一个 int 类型的信号，返回 void。

```  c
#include "csapp"
void sigint_handler(int sig)  //定义了一个信号处理程序
{
    printf("Caught SIGINT!\n");
    exit(0);
}
```

**安全的信号处理**

信号处理程序与主程序和其他信号处理程序并发地运行，如果它们并发地访问同样的全局数据结果，那结果可能不可预知。

一些保守的编写处理程序使其安全地并发运行的原则：

1. 处理程序要尽可能简单。越简单越不容易出错。
2. 在处理程序中只调用异步信号安全的函数。**异步信号安全的函数**能够被信号处理程序安全地调用，因为它们要么是可重入地，要么不能被信号处理程序中断。
3. 保存和恢复 errno。许多 Linux 异步信号安全的函数会在出错返回时设置 errno，在处理程序中调用时可能会干扰主程序中其他依赖于 errno 的部分。因此要在进入处理程序时将 errno 保存在局部变量中，返回前恢复它。
4. 阻塞所有的信号，保护对共享全局数据结构的访问。在访问处理程序和主程序共享的全局数据时，暂时阻塞信号。以保证信号处理程序不会中断那些用于访问数据的指令序列。
5. 用 **volatile** 声明全局变量。用 volatile 告诉编译器不要缓存这个变量，每次引用该变量都要从内存中读取。避免因为编译器的缓存优化机制导致处理程序和主程序获取的变量值不一样。
6. 用 sig_atomic_t 声明标志。详细介绍略，可以看书。

上面这些规则是保守的，不总是严格必须，但建议采用保守的方法。

下面是 Linux 保证异步信号安全的系统级函数，许多常见函数如 printf, malloc, exit 都不在其中，其中产生输出的唯一安全的函数是 write 函数。

​    ![18](Chapter 8/18.png)

因为产生输出的唯一安全的函数是 write 函数，本书作者开发了一些安全的函数，称为 SIO 包，用来在信号处理程序中打印简单的消息。

```        c
#include "csapp.h"
ssize_t sio_putl(long v);    //向标准输出打印一个 long 类型数字，如果成功返回传送的字节数，出错返回 -1
ssize_t sio_puts(char s[]);  //向标准输出打印一个字符串，如果成功返回传送的字节数，出错返回 -1
viod sio_error(char s[]);    //打印一条错误消息并终止。
```

**正确的信号处理**

要注意：如果存在一个未处理的信号 k，那表明至少有一个 k 信号到达了，实际上可能不止一个。

下面是一个忽略了这一点的错误的例子。

**一个错误的例子**

**SIGCHLD**：当子进程停止或终止时，会给父进程发送此信号，此信号的默认处理方式是忽略。

这个例子中，为了避免父进程使用 waidpid 回收子进程时挂起，采用 SIGCHLD 处理程序来回收子进程。

本例的实现原理：当有子进程停止或终止，父进程就会收到 SIGCHLD 信号，因此通过编写 SIGCHLD 的信号处理程序，在处理程序中使用 waidpid 完成子进程的回收，即可避免父进程的主程序被挂起。

```      c
void handler1(int sig)
{
    int olderrno = errno;  //保存 errno
    
    if((waitpid(-1, NULL, 0)) < 0)   //回收子进程
        sio_error("waidpid error");
    Sio_puts("Handler reaped child\n");
    Sleep(1);  // Sleep(1) 用来比喻回收子进程后要做的其他清理工作
    errno = olderrno;  //恢复 errno
}

int main()
{
    int i, n;
    char buf[MAXBUF];
    
    //将 handler1 函数设置为信号 SIGCHLD 的处理程序
    if(signal(SIGCHLD, handler1) == SIG_ERR)   //如果 signal 函数失败会返回 SIG_ERR
        unix_error("signal error");
        
    //创建子进程
    for(int i=0; i<3; i++)
    {
        if(Fork() == 0){                       //Fork() 是封装了错误处理的 fork() 函数
            printf("Hello from child %d\n", (int)getpid());
            exit(0);
        }
    }
    
    //父进程等待并处理终端输入————这部分就是父进程在回收子进程时想做的其他事情。
    if((n = read(STDIN_FILENO, buf, sizeof(buf))) < 0)
        unix_error("read");
    printf("Parent processing input\n");
    while(1);  // while(1) 用来比喻父进程一直在处理输入。
    
    exit(0);    
}
```

理解：信号处理程序不需要显式调用，当该进程收到了相关信息，会在自动调用信号处理程序。

理解：信号处理程序与主程序共享同样的全局数据，所以应该是线程级并发？

错误分析：这里有三个子进程，每个子进程终止时都会给父进程发送 SIGCHLD 信号，父进程收到 SIGCHLD 信号后就会调用一次信号处理程序，信号处理程序使用 waidpid() 函数来回收僵死进程。但是因为信号不会排队，如果在信号处理程序处理第一个僵死子进程时，后两个子进程也都终止了，那么第二个终止的子进程发给父进程的 SIGCHLD 信号会放到父进程的待处理信号集中，而第三个终止的子进程发送的 SIGCHLD 信号则会被丢弃。这样父进程就只能检测到两次 SIGCHLD 信号，这样就只能回收两个子进程了。

**问题修正**

问题分析：问题在于信号不会排队，即父进程检测到一个 SIGCHLD 信号时，可能终止的子进程不止一个。

解决方式：修改信号处理程序，让它在回收子进程时不要只回收一个，而是一次性回收所有僵死的子进程。

```   c
void handler2(int sig)
{
    int olderrno = errno;
    
    while(waitpid(-1, NULL, 0) > 0){         //循环回收所有僵死子进程。
        Sio_puts("Handler reaped child\n");
    }
    if(errno != ECHILD)  //当循环结束时，waitpid 的返回值为 -1。如果是因为已经没有了子进程而返回 -1，这时 errno 就被设置为了 ECHILD，这是正确的结果。如果不是 ECHILD，则说明出错了。
        Sio_error("waitpid error");
    Sleep(1);
    errno = olderrno;    
}
```

**可移植的信号处理**

Unix 信号处理的另一个缺陷：不同的系统有不同的信号处理语义。如：

1. signal 函数的语义各有不同。有些老的 Unix 系统在信号 k 被处理程序捕获后就把对信号 k 的反应恢复到默认值。
2. 系统调用可以被终端。像 read, write, accept 等系统调用潜在地会阻塞进程一段较长的时间，称为慢速系统调用。一些较早的 Unix 系统中，慢速系统调用被信号中断后不会继续执行，而是直接返回错误。

Posix 标准定义了 sigaction 函数，允许用户在设置信号处理时，明确指定想要的信号处理语义。

```      c
#include <signal.h>
int sigaction(int signum, struct sigaction *act, struct sigaction *oldact);  //若成功返回 0，出错返回 -1
```

sigaction 函数运用不广泛，常用的是使用一个调用了 sigaction 的包装函数 Signal。

Signal 包装函数设置了一个信号处理程序，它的信号处理语义如下：

1. 只有这个处理程序当前正在处理的那种类型的信号被阻塞。
2. 信号不会排队等待。
3. 只要可能，被中断的系统调用会自动重启。
4. 一旦设置了信号处理程序，它就会一直保持，直到 handler 参数为 SIG_IGN 或 SIG_DFL 的 Signal 函数被调用。

#### **8.5.6 同步流以避免讨厌的并发错误**

如何编写读写相同存储位置的并发流程序是一个难题。

基本的解决方式是：以某种方式同步并发流，从而得到最大的可行的交错集合，每个可行的交错都能得到正确的结果。

**一个具有细微同步错误的shell程序**

下面是一个典型的 Unix shell 的结构。父进程在一个全局作业列表中记录着它的当前子进程，每个作业一个条目。addjob 和 deletejob 函数分别向这个作业列表添加和从中删除作业。

当父进程创建一个子进程后，就把这个子进程添加到作业列表中。当父进程在 SIGCHLD 处理程序中回收一个僵死子进程时，它就从作业列表删除这个子进程。

```c
void handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all; //信号集合
    pid_t pid;
    
    Sigfillset(&mask_all); //把每个信号都添加到信号集合中
    while((pid = waitpid(-1, NULL, 0)) > 0)
    {
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); //删除作业前先阻塞所有信号
        deletejob(pid);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL); //删除完后恢复之前的阻塞信号集
    }
    if(errno != ECHILD)
        Sio_error("waitpid error");
    errno = olderrno;    
}

int main(int argc, char **argv)
{
    int pid;
    sigset_t mask_all, prev_all;
    
    Sigfillset(&mask_all);
    Signal(SIGCHLD, handler);
    initjobs();   //初始化作业列表
    
    while(1){
        if((pid = Fork()) == 0){
            Execve("/bin/date", argv, NULL);
        }
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        addjob(pid);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    exit(0);
}
```

这个程序具有一些细微的错误。

它可能发生下面这样的事件序列：

1. 父进程执行 fork 函数，内核调度新创建的子进程运行，而不是运行父进程。
2. 在父进程能够再次运行前，子进程就终止，变成一个僵死进程，使内核传递一个 SIGCHLD 信号给父进程。
3. 当父进程再次变成可运行状态，在它正式执行之前，内核注意到有未处理的 SIGCHLD 信号，并通过在父进程中运行信号处理程序来接收这个信号。
4. 信号处理程序回收终止的子进程，并调用 deletejob，但实际上 deletejob 函数什么也不会做，因为父进程还没有把这个子进程添加到作业列表中。
5. 信号处理程序运行完毕后，内核运行父进程，父进程从 fork 返回，调用 addjob 函数错误地把这个已经终止并被回收掉的子进程添加到作业列表中。

上面的事件序列中，父进程的 main 程序和信号处理流的错误交错，使得在 addjob 之前调用了 deletejob，导致在作业列表中出现了一个不正确的条目。

这是一种经典的同步错误：**竞争**。main 函数中调用 addjob 和信号处理程序中调用 deletejob 之间存在竞争，如果 addjob 赢得竞争，结果就是正确的，反之，结果就是错误的。

**消除竞争**

通过在调用 fork 之前，阻塞 SIGCHLD 信号，然后在调用 addjob 之后取消阻塞这些信号，保证了在子进程被添加到作业列表之后才可能回收该子进程。

注意：子进程会继承父进程的被阻塞集合，所以必须在调用 execve 之前，解除子进程中阻塞的 SIGCHLD 信号。

```          c
void handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    
    Sigfillset(&mask_all);
    while((pid = waitpid(-1, NULL, 0)) > 0){
        Sigprocmask(SIG_BLOCK, mask_all, prev_all);
        deletejob(pid);
        Sigprocmask(SIG_SETMASK, prev_all, NULL);
    }
    if(errno != ECHILD)
        Sio_error("waitpid error");
    errno = olderrno;
}
int main(int argc, char **argv)
{
    int pid;
    sigset_all mask_all, mask_one, prev_one;
    
    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIG_CHLD);
    Signal(SIG_CHLD, handler);
    initjobs();
    
    while(1) {
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        if((pid = Fork()) == 0){
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
            Execve('/bin/date', argv, NULL);
        }
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(pid);
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
    }
    exit(0);
}
```

#### **8.5.7 显式地等待信号**

有时候主程序需要显式地等待某个信号处理程序运行。如当 Linux shell 创建一个前台作业时，在接收下一条用户命令之前，它必须等待作业终止，被 SIGCHLD 处理程序回收。

```c
#include "csapp.h"

volatile sig_atomic_t pid;

void sigchld_handler(int s)
{
    int olderrno = errno;
    pid = waitpid(-1, NULL, 0); //SIG_CHLD 的信号处理程序修改了 pid 的值。
    errno = olderrno;
}

void sigint_handler(int s) {}

int main(int argc, char **argv)
{
    sigset_t mask, prev;
    
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    
    while(1){
        Sigprocmask(SIG_BLOCK, &mask, &prev);  //阻塞 SIGCHLD 信号
        if(Fork() == 0)
            exit(0);
        
        pid = 0; //在阻塞 SIG_CHLD 信号期间，将 pid 置为 0。
        Sigprocmask(SIG_SETMASK, &prev, NULL); //取消阻塞 SIGCHLD 信号
        
        while(!pid) ;  //这个无限循环就是在显式地等待作业终止并被信号 SIGCHLD 的处理程序回收。这里的等待过程一直在占用 CPU，实际上是很浪费资源的。
        printf(".")        
    }
    exit(0);
}
```

如何避免 while(!pid) 的等待过程浪费资源？

解决方法1：在循环体内插入 pause

``````     c
hile(!pid) pause(); 
``````

有问题，有严重的竞争条件：如果在 while 测试后和 pause 之前收到 SIGCHLD 信号，pause 会永远暂停。

解决方法2：在循环体内插入 sleep

``````c
while(!pid) sleep(1);
``````

有问题，太慢了，每次循环检查后需要等很长时间才会再次检查循环条件。              

合适的解决方法：使用 sigsuspend 函数

 ```   c
 #include <signal.h>
 int sigsuspend(const sigset_t *mask);  //返回 -1
 ```

sigsuspend 函数等价于下述代码的原子（不可中断的）版本，它暂时用 mask 替换当前的阻塞集合，然后挂起该进程，直到收到一个信号。

```        c
sigprocmask(SIG_SETMASK, &mask, &prev);
pause();
sigprocmask(SIG_SETMASK, &prev, NULL);
```

使用 sigsuspend 来修改上面的 shell 例子，既可以避免 pause 带来的竞争，又比 sleep 快很多。

```c
int main(int argc, char **argv)
{
    sigset_t mask, prev;
    
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    
    while(1){
        Sigprocmask(SIG_BLOCK, &mask, &prev);  //阻塞 SIGCHLD 信号
        if(Fork() == 0)
            exit(0);
        pid = 0;
        while(!pid)
            sigsuspend(&prev);  //在 sigsuspend 中取消阻塞 SIGCHLD 信号并等待作业终止，等作业终止后恢复之前的阻塞集合。
        Sigpromask(SIG_SETMASK, &prev, NULL);  //取消阻塞 SIGCHLD 信号。
        printf(".");        
    }
    exit(0);
}
```

### **8.6 非本地跳转**

C 语言提供了一种用户级异常控制流形式，称为**非本地跳转**，它将控制直接从一个函数转移到另一个当前正在执行的函数，而不需要经过正常的调用-返回序列（即 C 标准库 **setjmp.h** 的内容）。

非本地跳转通过 **setjmp 和 longjmp 函数**来完成

**sigjmp函数**

```          c
#include<setjmp.h>   //c 标准库中的头文件
int setjmp(jmp_buf env);  //返回 0，setjmp 的返回值不能被赋值给变量，但可以用在条件语句的测试中
int sigsetjmp(sigjmp_buf env, int savesigs);
```

setjmp 函数在 env 缓冲区中保存当前调用环境，以供后面的 longjmp 使用。

调用环境包括程序计数器、栈指针和通用目的寄存器。

**longjmp函数**

```c
void longjmp(jmp_buf env, int retval);
void siglongjmp(sigjmp_buf env, int retval);
```

longjmp 函数从 env 缓冲区中恢复调用环境，然后触发一个从最近一次初始化 env 的 setjmp 调用的返回。然后 setjmp 返回，并带有非零的返回值 retval。

setjmp 和 longjmp 之间的关系比较复杂：**setjmp 函数只被调用一次，但返回多次**：一次是第一次调用 setjmp 将调用环境保存在 env 中时，一次是为每个相应的 longjmp 调用。而 **longjmp 函数被调用一次，但从不返回**。

非本地跳转的一个重要应用：允许从一个深层嵌套的函数调用中立即返回，而不需要解开整个栈的基本框架，通常是由检测到某个错误情况引起的。

C++ 和 Java 提供的 try 语句块异常处理机制是较高层次的，是 C 语言的 setjmp 和 longjmp 函数的更加结构化的版本。可以把 throw 语句看作 longjmp 函数，catch 子句看作 setjmp 函数。

### **8.7 操作进程的工具**

Linux 系统提供了大量的监控和操作进程的工具：

1. STRACE：打印一个正在运行的程序和它的子进程调用的每个系统调用的轨迹。
2. PS：列出当前系统中的进程（包括僵死进程）。
3. TOP：打印关于当前进程资源使用的信息。
4. PMAP：显示进程的内存映射。
5. **/proc：一个虚拟文件系统**，以 ASCII 文本格式输出大量内核数据结构的内容，用户程序可以读取这些内容。