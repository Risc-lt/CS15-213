#  Shell Lab

### Content

Looking at the tsh.c (tiny shell) file, you will see that it contains a functional skeleton of a simple Unix shell. 

To help you get started, we have already implemented the less interesting functions. Your assignmentis to complete the remaining empty functions listed below. As a sanity check for you, we’ve listed the approximate number of lines of code for each of these functions in our reference solution (which includeslots of comments).

- eval: Main routine that parses and interprets the command line. 
- builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs.
- do bgfg: Implements the bg and fg built-in commands. 
- waitfg: Waits for a foreground job to complete. 
- sigchld handler: Catches SIGCHILD signals.
- sigint handler: Catches SIGINT (ctrl-c) signals. 
- sigtstp handler: Catches SIGTSTP (ctrl-z) signals.

简单来说，最终目的就是要实现一个名叫 `tsh` 的类shell 交互式命令行解释器，来代表用户运行程序。

需要实现的功能包括而不限于像读取和解析命令行参数，前台与后台工作的执行与调度，创建与回收子进程以及处理kernel或其它进程传输的信号这些基本操作

### Preparation

根据writeup上的指导，需要完成的部分有：

- `eval`：解析并执行命令行输入的主进程。
- `builtin_cmd`：检测命令是否为内置的命令（在本lab中需要实现的命令有：`kill`, `fg`, `bg` 和 `jobs`）
- `do_bgfg`：实现 `bg` 和 `fg` 这两个内置指令，也就是前台和后台运行的两种情形。
- `waitfg`：等待一个前台任务结束。
- `sigchld_handler`：处理 `SIGCHLD` 信号的信号处理函数。
- `sigint_handler`：处理 `SIGINT(Ctrl-C)` 信号的信号处理函数。
- `sigtstp_handler`：处理 `SIGTSTP(Ctrl-Z)` 信号的信号处理函数。

此外，Shell 的多个内建方法的功能为：

- `jobs`：列出正在运行和停止的后台作业。
- `bg <job>`：将一个停止的作业转入后台开始运行。
- `fg <job>`：将一个停止或者正在运行的后台作业转入前台进行运行。
- `kill <job>`：终止一个作业。（这个貌似没要求）
- `quit`：退出终端程序

TIPS:

1. 用户的输入包括 `name` 加上零个或多个参数，这些参数之间用一个或多个空格分隔。如果 `name` 是内置命令，那么直接执行然后等待下一个命令，否则 Shell 需要认为 `name` 是一个可执行文件的路径，需要新建一个子进程，并在子进程中完成具体的工作。

2. 不需要支持管道或者 I/O 重定向。

3. 如果输入的命令以 `&` 结尾，那么就要以后台任务的方式执行，否则按照前台执行。

4. 每个作业都有其进程 ID (PID) 和 job ID (JID)，都是由 tsh 指定的正整数，JID 以 `%` 开头（如 `%5` 表示 JID 为 5，而 `5` 则表示 PID 为 5）。

5. tsh 应该回收所有的僵尸进程，如果任何job因为接收了没有捕获的信号而终止，tsh 应该识别出这个时间并且打印出 JID 和相关信号的信息。

6. 编写安全的信号处理程序

   - 处理程序尽可能的简单。

     避免麻烦的最好方法是保持处理程序尽可能的小和简单。例如，处理程序可能只是简单地设置全局标志并立即返回；所有与接收信号相关的处理程序都由主程序执行，它周期性地检查（并重置）这个标志。

   - 在处理程序中只调用异步信号安全的函数。

     所谓异步安全的函数能够被信号处理程序安全的调用，原因有二：要么它是可重入的（例如只访问局部变量），要么它不能被信号处理程序中断。很多常见的函数都不是安全的，例如 `printf`, `sprintf`, `malloc` 和 `exit`，以及，在进行fork或pause操作时，为了防止可能的突然信号影响，需要先设置掩码来阻塞信号，最后再重新释放。

   - 保存和恢复 `errno`。

     许多 Linux 异步安全的函数都会在出错返回时设置 `errno`。在处理程序中调用这样的函数可能会干扰主程序中其他依赖于 `errno` 的部分。解决办法是在进入处理程序时把 `errno` 保存在一个局部变量中，在处理函数返回前恢复它。注意，只有在处理程序需要返回时才有此必要。如果处理程序调用`_exit` 终止该进程，那么就不需要这样做了。

   - 阻塞所有的信号，保护对共享全局数据结构的访问。

     如果处理程序和主程序或其他处理程序共享一个全局数据结构，那么在访问（读或者写）该数据结构时，你的处理程序和主程序应该暂时阻塞所有的信号。这条规则的原因是从主程序访问一个数据结构 `d` 通常需要一系列指令，如果指令序列被访问 `d` 的处理程序中断，那么处理程序可能会发现 `d` 的状态不一致，得到不可预知的结果。在访问 `d` 时暂时阻塞信号保证了程序不会中断该指令。

   - 用 `volatile` 声明全局变量。

     `volatile` 限定符强迫编译器每次在代码中引用 `g` 时，从内存中读取 `g` 的值。一般来说，和其他所有共享数据结构一样，应该暂时阻塞信号，保护每次对全局变量的访问。

   - 用 `sig_atomic_t` 声明标志。

     在常见的处理程序设计中，处理程序会写全局标志来记录收到了信号。主程序周期性的读取这个标志，相应信号，再清除该标志。对于通过这种方式来共享的标志，C 提供一种整型数据类型 `sig_atomic_t`，对它的读和写保证会是原子的，

### Puzzles

1. `waitfg`

   waitfg的效果很简单，就是在前台作业结束前以及无前台作业的情况下在命令交互处等待。

   执行逻辑选择的是自旋，使用 `fgjob` 比较在前台的作业 PID 和传入 PID，相同则忽略，不同则执行。需要注意的点是，如果循环条件直接设置为比较的话会消耗占用很多处理器资源，所以我们改为用全局变量的形式记录；此外，如果在 `while` 循环刚检测完 `fgchld_flag` 时决定进入循环时突然收到打断的 `SIGCHLD` 直接修改了 `fgchld_flag` 值，就没能达成通知的目的，程序会一直卡死在 `pause` 无法醒来，造成死锁，所以我们选用`sigsuspend`来进行信号阻塞和回复

2. `sigint_handler` / `sigtstp_handler`

   sigint_handler和 sigtstp_handler的效果在这里差不多，都是进行前台进程或工作的中断。

   执行逻辑就是先找到前台执行的job的PID或JID，然后利用`kill`向这个job传输停止信号即可。唯一需要注意的是，前台无正在进行的工作时，需要跳过这一步操作；以及需要访问全局数据结构 `jobs`的时候需要对所有信号在此期间进行阻塞。

3. `sigchld_handler`

   sigchld_handler的效果是回收已经结束或者处于zombie状态的子进程，但是`SIGCHLD` 不是只有子进程终止时这一种情形。`SIGCHLD` 信号产生的条件有：

   - 子进程终止时会向父进程发送 `SIGCHLD` 信号，告知父进程回收自己，但该信号的默认处理动作为忽略，因此父进程仍然不会去回收子进程，需要捕捉处理实现子进程的回收；
   - 子进程接收到 `SIGSTOP` 信号停止时；
   - 子进程处在停止态，接受到 `SIGCONT` 后唤醒时。

   而此处的注释中提到，不需要我们等待其他正在运行的进程终止，即不阻塞，所以考虑在 `waitpid` 的 `options` 参数中加入 `WNOHANG` 来立即执行

   整体的执行逻辑就是先等待任意子进程改变状态，如果是前台进程，修改标志，通知waitfg，然后检查如果是正常退出，则直接从作业表中删除当前子进程；如果是收到信号终止，打印消息；如果是被暂停，在作业表中将作业状态改为暂停，并打印消息。同样的，在访问全局数据结构 `jobs`的时候需要对所有信号在此期间进行阻塞，以及保留当前上下文环境中异步操作产生的errno，记得在逻辑的最后恢复；

4. `builtin_cmd`

   builtin_cmd的效果是调用内置函数，所以我们需要补充函数的执行内容，并用条件判断来选择调用即可

   其中：

   `jobs`指令代表主进程会调用 `listjobs` 函数，将作业表 `jobs` 中的所有内容打印出来；

   `fg <job> ` / `bg <job>` 命令通过向 <job> 发送 SIGCONT 信号重启 <job>，然后在前台 / 后台运行 <job>；实质上只需要我们调用`do_bgfg`；

   `kill`命令可以利用内置已经完成好的错误封装函数直接实现（需要注意的是，kill不只是能结束进程，还可以对当前前台任务及其子进程发送信号来执行其他操作）

5. `do_bgfg`

   `do_bgfg`的效果是执行内置的 bg 和 fg 命令，但由于我们需要根据参数的特征来确定是 PID还是 JID，以及处理参数 PID/JID 均不匹配，没有找到 PID/JID 对应的作业等异常情况，所以需要加上一些判断的预处理

   故而，主体部分的执行逻辑大体上是先向PID对应的job利用`Kill`传输`SIGCONT`信号，如果该工作处于后台，则设置状态，打印；如果该工作处于前台，则设置状态，挂起。同样，我们也要注意利用掩码来进行信号阻塞，从而保护全局变量和保存errno

6. `eval`

   `eval` 则是 shell 的主处理函数，用来识别和执行非内嵌函数

   执行逻辑则是首先根据解析好的命令行，判断程序是否以`&`结尾来确定运行位置在前台/后台，存储进bg的判断符中，然后判断指令的类型：如果是内置指令，则不需要我们进行操作；如果是非内置指令，我们先fork出来子进程，在子进程中令进程组号等于进程号，再利用Execve(argv[0], argv, environ)来执行命令路径，然后返回父进程，将该job添加至工作条目，最后根据bg的值决定前台是否挂起即可。同样，我们需要注意，为了防止在操作中子进程就收到 `SIGCHLD` 信号被打断，我们需要设置掩码进行阻塞，同时在执行完主逻辑后回复信号通路。
