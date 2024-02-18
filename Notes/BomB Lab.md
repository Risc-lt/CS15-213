# Bomb Lab

### Content

**Your job for this lab is to defuse your bomb.**

（There are several other tamper-proofing devices built into the bomb as well, or so we hear.）

You can use many tools to help you defuse your bomb. The best way is to **use your favorite debugger to step through the disassembled binary**.Each time your bomb explodes it notifies the bomblab server, and you lose 1/2 point (up to a max of 20 points) in the final score for the lab. So there are consequences to exploding the bomb. **You must be careful!**

The first four phases are worth 10 points each. Phases 5 and 6 are a little more difficult, so they are worth 15 points each. So the maximum score you can get is 70 points.

The bomb ignores blank input lines.In a moment of weakness, Dr. Evil added this feature so you don’t have to keep retyping the solutions to phases you have already defused.

To avoid accidentally detonating the bomb, you will need to learn how to **single-step through the assembly code** and how to **set breakpoints**. You will also need to learn how to inspect both the registers and the memory states. One of the nice side-effects of doing the lab is that you will get very good at **using a debugger**. 

**Tips**:  If you run your bomb with a command line argument, for example,
`linux> ./bomb psol.txt`
then it will read the input lines from psol.txt until it reaches EOF (end of file), and then switch over to stdin. 

### Logitics

Save the `bombk.tar` file to a (protected) directory in which you plan to do your work. Then give the

command: `tar -xvf bombk.tar.` This will create a directory called ./bombk with the following

files:

- README: Identifies the bomb and its owners.
- bomb: The executable binary bomb.
-  bomb.c: Source file with the bomb’s main routine and a friendly greeting from Dr. Evil.
- writeup.{pdf,ps}: The lab writeup

然而，以上文件唯一的用处不过是提供bomb的disasemble文件（x

### Summary

#### Debbuger:

在开始之前，首先要熟悉debugger的使用方法，下面是笔者在lab期间常用的指令：

|         命令          |                      功能                      |
| :-------------------: | :--------------------------------------------: |
|     gdb filename      |                    开始调试                    |
|          run          |                    开始运行                    |
|       run 1 2 3       |          开始运行,并且传入参数1，2，3          |
|         kill          |                    停止运行                    |
|         quit          |                    退出gdb                     |
|       break sum       |            在sum函数的开头设置断点             |
|    break *0x8048c3    |           在0x8048c3的地址处设置断点           |
|       delete 1        |                   删除断点1                    |
|       clear sum       |            删除在sum函数入口的断点             |
|         stepi         |                  运行一条指令                  |
|        stepi 4        |                  运行4条指令                   |
|       continue        |                运行到下一个断点                |
|       disas sum       |                 反汇编sum函数                  |
|     disas 0X12345     |           反汇编入口在0x12345的函数            |
|  print /x /d /t $rax  | 将rax里的内容以16进制，10进制，2进制的形式输出 |
|      print 0x888      |             输出0x888的十进制形式              |
| print *(int*)0x123456 |    将0x123456地址所存储的内容以数字形式输出    |
| print (char*)0x123456 |           输出存储在0x123456的字符串           |
|       x/w $rsp        |           解析在rsp所指向位置的word            |
|       x/2w $rsp       |         解析在rsp所指向位置的两个word          |
|      x/2wd $rsp       |  解析在rsp所指向位置的word，以十进制形式输出   |
|    info registers     |                   寄存器信息                   |
|    info functions     |                    函数信息                    |
|      info stack       |                     栈信息                     |

其它更详细的指令资料请见[Linux Tools-gdb](https://linuxtools-rst.readthedocs.io/zh-cn/latest/tool/gdb.html)

#### Defusion

前情提要：笔者的开发环境为Ubuntu 20.04.6 LTS on Windows 10 x86 64（以下指令仅可适用于Unix系统）

##### Preparation

这里首先需要将二进制的执行文件bomb进行反汇编

- 可以选择将bomb反编译为.s的汇编文件单独储存：

```bash
$objdump -d bomb > bomb.s
```

- 也可以直接在terminal中进行操作来进行部分查看，比如打开符号表：

```bash
$objdump -t bomb | less
```

然后，你就会看到一大堆汇编操作，这时候不要和笔者一样傻眼了，大体浏览一下就会发现writeup里所说的6个phase，这就是lab中需要破解的6个bomb，于是我们就可以开始紧张刺激的拆弹了

##### Phase_1

```assembly
0000000000400ee0 <phase_1>:
  400ee0: 48 83 ec 08           sub    $0x8,%rsp       # 准备栈空间，将栈指针 %rsp 减去8字节
  400ee4: be 00 24 40 00        mov    $0x402400,%esi  # 将立即数0x402400移动到寄存器 %esi 中
  400ee9: e8 4a 04 00 00        callq  401338 <strings_not_equal>  # 调用函数 
  400eee: 85 c0                 test   %eax,%eax       # 对寄存器%eax进行与运算，设置相关标志位
  400ef0: 74 05                 je     400ef7 <phase_1+0x17># 如果%eax为零则跳转地址0x400ef7
  400ef2: e8 43 05 00 00        callq  40143a <explode_bomb>       # 炸弹爆炸
  400ef7: 48 83 c4 08           add    $0x8,%rsp       # 恢复栈指针，相当于撤销之前的栈空间分配
  400efb: c3                    retq                    		   # 返回
```

​		phase_1相对简单，过程的大体逻辑是先读取输入的字符串，然后调用strings_not_equal函数直接判断两个字符串是否相等，其中第二个字符串起始地址直接存储在%esi里，作为进行比较的参数传入，所以可以直接用检查0x402400的存储值，输入 `x $esi` 就可以得到答案：

```
Border relations with Canada have never been better.
```

##### Phase_2

```assembly
0000000000400efc <phase_2>:
  400efc: 55                    push   %rbp   # 保存 %rbp 寄存器的值，将其压入栈中
  400efd: 53                    push   %rbx   # 保存 %rbx 寄存器的值，将其压入栈中
  400efe: 48 83 ec 28           sub    $0x28,%rsp # 为局部变量分配 40 字节的栈空间
  400f02: 48 89 e6              mov    %rsp,%rsi  # 栈指针 %rsp 的值赋给 %rsi 寄存器
  400f05: e8 52 05 00 00        callq  40145c <read_six_numbers>  # 调用函数 
  400f0a: 83 3c 24 01           cmpl   $0x1,(%rsp)          # 比较栈顶的值和1
  400f0e: 74 20                 je     400f30 <phase_2+0x34>  # 如果相等，则跳转
  400f10: e8 25 05 00 00        callq  40143a <explode_bomb>   # 如果不相等，则炸弹爆炸
  400f15: eb 19                 jmp    400f30 <phase_2+0x34>  # 无条件跳转到地址0x400f30
  400f17: 8b 43 fc              mov    -0x4(%rbx),%eax   # 将 %rbx 寄存器指向的内存位置的值移动到 %eax 寄存器
  400f1a: 01 c0                 add    %eax,%eax     # 将 %eax 寄存器的值加倍
  400f1c: 39 03                 cmp    %eax,(%rbx)   # 比较加倍后的值与 %rbx 寄存器指向的内存位置的值
  400f1e: 74 05                 je     400f25 <phase_2+0x29>  # 如果相等，则跳转
  400f20: e8 15 05 00 00        callq  40143a <explode_bomb>   # 如果不相等，则炸弹爆炸
  400f25: 48 83 c3 04           add    $0x4,%rbx  # 将 %rbx 寄存器的值增加4（移动到下一个元素）
  400f29: 48 39 eb              cmp    %rbp,%rbx  # 比较 %rbx 寄存器的值与 %rbp 寄存器的值
  400f2c: 75 e9                 jne    400f17 <phase_2+0x1b>  # 如果不相等，则跳转
  400f2e: eb 0c                 jmp    400f3c <phase_2+0x40>  # 无条件跳转到地址0x400f3c
  400f30: 48 8d 5c 24 04        lea    0x4(%rsp),%rbx # 计算地址，将 %rbx 寄存器设置为栈顶的地址 + 4
  400f35: 48 8d 6c 24 18        lea    0x18(%rsp),%rbp       # 计算地址，将 %rbp 寄存器设置为栈顶的地址 + 24
  400f3a: eb db                 jmp    400f17 <phase_2+0x1b>  # 无条件跳转到地址0x400f17
  400f3c: 48 83 c4 28           add    $0x28,%rsp   # 恢复栈指针，撤销之前为局部变量分配的栈空间
  400f40: 5b                    pop    %rbx         # 弹出栈中保存的 %rbx 寄存器的值
  400f41: 5d                    pop    %rbp         # 弹出栈中保存的 %rbp 寄存器的值
  400f42: c3                    retq                # 返回

```

​        phase_2主要考查了对栈顶数组的操作，入手点在于调用的read_six_numbers这个函数，显然这次要求的输入是6个数字，根据第八行和第九行，第一个数字必须是1，否则会爆炸，我们观察到如果第一个数字是1，跳转到`400f30`，也就是`lea 0x4(%rsp),%rbx`，%rsp是栈指针的地址，每个int的数据长度为4个bytes，这句话的意思就是说读取下一个数字的地址，存入%rbx里。下一行有个奇怪的数值0x18，十进制为24，24/4=6正好是6个数字，于是我们发现这一行的目的就是设置一个结束点，放在%rbp中，然后回到`400f17` 循环，所有最后我们发现这段程序是在循环判断一个数组是否为公比为2的等比数列，且第一个数必须是1，答案就显而易见了：

```
1 2 4 8 16 32
```

##### Phase_3

```assembly
0000000000400f43 <phase_3>:
    400f43: 48 83 ec 18           sub    $0x18,%rsp  # 为局部变量分配24字节的栈空间
    400f47: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx # 计算地址，将 %rcx 寄存器设置为栈顶的地址 + 12
    400f4c: 48 8d 54 24 08        lea    0x8(%rsp),%rdx # 计算地址，将 %rdx 寄存器设置为栈顶的地址 + 8
    400f51: be cf 25 40 00        mov    $0x4025cf,%esi # 将立即数0x4025cf移动到寄存器 %esi 中
    400f56: b8 00 00 00 00        mov    $0x0,%eax      # 将立即数0移动到寄存器 %eax 中
    400f5b: e8 90 fc ff ff        callq  400bf0 <__isoc99_sscanf@plt>  # 调用函数 sscanf 
    400f60: 83 f8 01              cmp    $0x1,%eax      # 比较 %eax 寄存器的值和1
    400f63: 7f 05                 jg     400f6a <phase_3+0x27>  # 如果大于1，则跳转
    400f65: e8 d0 04 00 00        callq  40143a <explode_bomb>   # 如果不大于1，则炸弹爆炸
    400f6a: 83 7c 24 08 07        cmpl   $0x7,0x8(%rsp) # 比较栈上偏移8的地址处的值和7
    400f6f: 77 3c                 ja     400fad <phase_3+0x6a>  # 如果大于7，则跳转
    400f71: 8b 44 24 08           mov    0x8(%rsp),%eax # 将栈上偏移8的地址处的值移动到 %eax 寄存器
    400f75: ff 24 c5 70 24 40 00  jmpq   *0x402470(,%rax,8)   # 间接跳转，根据 %rax 寄存器的值选择跳转目标
    400f7c: b8 cf 00 00 00        mov    $0xcf,%eax            # 跳转目标1
    400f81: eb 3b                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400f83: b8 c3 02 00 00        mov    $0x2c3,%eax           # 跳转目标2
    400f88: eb 34                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400f8a: b8 00 01 00 00        mov    $0x100,%eax           # 跳转目标3
    400f8f: eb 2d                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400f91: b8 85 01 00 00        mov    $0x185,%eax           # 跳转目标4
    400f96: eb 26                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400f98: b8 ce 00 00 00        mov    $0xce,%eax            # 跳转目标5
    400f9d: eb 1f                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400f9f: b8 aa 02 00 00        mov    $0x2aa,%eax           # 跳转目标6
    400fa4: eb 18                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400fa6: b8 47 01 00 00        mov    $0x147,%eax           # 跳转目标7
    400fab: eb 11                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400fad: e8 88 04 00 00        callq  40143a <explode_bomb>   # 如果栈上偏移8的地址处的值大于7，则调用 explode_bomb 函数
    400fb2: b8 00 00 00 00        mov    $0x0,%eax             # 跳转目标8
    400fb7: eb 05                 jmp    400fbe <phase_3+0x7b>  # 无条件跳转到地址0x400fbe
    400fb9: b8 37 01 00 00        mov    $0x137,%eax           # 跳转目标9
    400fbe: 3b 44 24 0c           cmp    0xc(%rsp),%eax        # 比较栈上偏移12的地址处的值和 %eax 寄存器的值
    400fc2: 74 05                 je     400fc9 <phase_3+0x86>  # 如果相等，则跳转到地址0x400fc9
    400fc4: e8 71 04 00 00        callq  40143a <explode_bomb>   # 如果不相等，则调用 explode_bomb 函数
    400fc9: 48 83 c4 18           add    $0x18,%rsp            # 恢复栈指针，相当于撤销之前为局部变量分配的栈空间
    400fcd: c3                    retq                           # 返回，从先前的栈中弹出返回地址，将控制权交还给调用者   
```

​		phase_3主要考察类似`switch`的条件判断分支，破局点在调用的sscanf函数，这个函数是用来解析和匹配字符串的，于是我们输入 `print (char*)0x4025cf` ，就可以发现格式字符串为 `%d %d` ，也就是两个int数字。接下来 `400f6f` 处读取第一个数字，且从下面一行可以看出第一个数字必须<=7，否则也会引爆炸弹，再往下发现该参数是用于实现跳转语句，根据它跳转到不同的行数并获取一个值放入%eax里，要求第二个数必须等于放入的值，于是我们选择最简单的0，就能获得答案：(本问是多解，第一个参数0~7均可，后一个参数根据跳转算得即可)

```
0 207
```

##### Phase_4

```assembly
000000000040100c <phase_4>:
  40100c: 48 83 ec 18           sub    $0x18,%rsp            # 为局部变量分配24字节的栈空间
  401010: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx        # 计算地址，将 %rcx 寄存器设置为栈顶的地址 + 12
  401015: 48 8d 54 24 08        lea    0x8(%rsp),%rdx        # 计算地址，将 %rdx 寄存器设置为栈顶的地址 + 8
  40101a: be cf 25 40 00        mov    $0x4025cf,%esi        # 将立即数0x4025cf移动到寄存器 %esi 中
  40101f: b8 00 00 00 00        mov    $0x0,%eax             # 将立即数0移动到寄存器 %eax 中
  401024: e8 c7 fb ff ff        callq  400bf0 <__isoc99_sscanf@plt>  # 调用函数 sscanf 解析输入
  401029: 83 f8 02              cmp    $0x2,%eax             # 比较 %eax 寄存器的值和2
  40102c: 75 07                 jne    401035 <phase_4+0x29>  # 如果不等于2，则跳转到地址0x401035
  40102e: 83 7c 24 08 0e        cmpl   $0xe,0x8(%rsp)        # 比较栈上偏移8的地址处的值和14
  401033: 76 05                 jbe    40103a <phase_4+0x2e>  # 如果小于等于14，则跳转到地址0x40103a
  401035: e8 00 04 00 00        callq  40143a <explode_bomb>   # 如果不等于2或栈上偏移8的地址处的值大于14，则调用 explode_bomb 函数
  40103a: ba 0e 00 00 00        mov    $0xe,%edx            # 将立即数14移动到寄存器 %edx 中
  40103f: be 00 00 00 00        mov    $0x0,%esi            # 将立即数0移动到寄存器 %esi 中
  401044: 8b 7c 24 08           mov    0x8(%rsp),%edi       # 将栈上偏移8的地址处的值移动到寄存器 %edi 中
  401048: e8 81 ff ff ff        callq  400fce <func4>        # 调用函数 func4
  40104d: 85 c0                 test   %eax,%eax            # 对 %eax 寄存器进行与运算，设置相关的标志位
  40104f: 75 07                 jne    401058 <phase_4+0x4c>  # 如果 %eax 不为零，则跳转到地址0x401058
  401051: 83 7c 24 0c 00        cmpl   $0x0,0xc(%rsp)        # 比较栈上偏移12的地址处的值和0
  401056: 74 05                 je     40105d <phase_4+0x51>  # 如果相等，则跳转到地址0x40105d
  401058: e8 dd 03 00 00        callq  40143a <explode_bomb>   # 如果 %eax 不为零或栈上偏移12的地址处的值不为零，则调用 explode_bomb 函数
  40105d: 48 83 c4 18           add    $0x18,%rsp            # 恢复栈指针，相当于撤销之前为局部变量分配的栈空间
  401061: c3                    retq                           # 返回，从先前的栈中弹出返回地址，将控制权交还给调用者

```

​		phase_4上了一点难度，第一个参数可以直接从 `40102e`处的`cmpl  $0xe,0x8(%rsp)`里得到，关键点在于如何理解递归函数 `func4`：

```assembly
0000000000400fce <func4>:
  400fce:	48 83 ec 08          	sub    $0x8,%rsp
  400fd2:	89 d0                	mov    %edx,%eax
  400fd4:	29 f0                	sub    %esi,%eax
  400fd6:	89 c1                	mov    %eax,%ecx
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx
  400fdb:	01 c8                	add    %ecx,%eax
  400fdd:	d1 f8                	sar    %eax
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx
  400fe2:	39 f9                	cmp    %edi,%ecx
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>
  400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
  400fe9:	e8 e0 ff ff ff       	callq  400fce <func4>
  400fee:	01 c0                	add    %eax,%eax
  400ff0:	eb 15                	jmp    401007 <func4+0x39>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax
  400ff7:	39 f9                	cmp    %edi,%ecx
  400ff9:	7d 0c                	jge    401007 <func4+0x39>
  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	retq   
```

​		这里建议一行一行跟着动手算一算，最后能够简化出下面的c代码：

```c
int func4(int n, int i, int j) {
    int val = (j + i) / 2;
    if (val <= n) {
        if (val >= n) return 0;
        else return 2 * func4(n, val+1, j) + 1;
    } else {
        return 2 * func4(n, i, val-1);
    }
}
```

​		根据`401051` 处的 `cmpl  $0x0, 0xc(%rsp) `指令可知，`func4` 的返回值必须为0，所以第二个参数确定为0，最后其中一个答案为：

```
0 0
```

##### Phase_5

```assembly
0000000000401062 <phase_5>:
  401062: 53                    push   %rbx        # 保存 %rbx 寄存器的值
  401063: 48 83 ec 20           sub    $0x20,%rsp  # 为局部变量分配32字节的栈空间
  401067: 48 89 fb              mov    %rdi,%rbx   # 将参数 %rdi 的值移动到 %rbx 寄存器中
  40106a: 64 48 8b 04 25 28 00  mov    %fs:0x28,%rax   # 获取当前线程的栈底地址
  401071: 00 00 
  401073: 48 89 44 24 18        mov    %rax,0x18(%rsp) # 将栈底地址保存在栈上
  401078: 31 c0                 xor    %eax,%eax       # 将 %eax 寄存器清零
  40107a: e8 9c 02 00 00        callq  40131b <string_length>   # 调用函数 
  40107f: 83 f8 06              cmp    $0x6,%eax        # 比较字符串长度是否为6
  401082: 74 4e                 je     4010d2 <phase_5+0x70>    # 如果是，则跳转到地址0x4010d2
  401084: e8 b1 03 00 00        callq  40143a <explode_bomb>    # 如果不是6，则调用 explode_bomb 函数
  401089: eb 47                 jmp    4010d2 <phase_5+0x70>    # 跳转到地址0x4010d2，不执行后续指令
  40108b: 0f b6 0c 03           movzbl (%rbx,%rax,1),%ecx       # 从字符串中读取一个字节到 %ecx 寄存器
  40108f: 88 0c 24              mov    %cl,(%rsp)               # 将 %cl 寄存器的值移动到栈上
  401092: 48 8b 14 24           mov    (%rsp),%rdx              # 读取栈上的值到 %rdx 寄存器
  401096: 83 e2 0f              and    $0xf,%edx                # 将 %edx 寄存器的值与15进行按位与操作
  401099: 0f b6 92 b0 24 40 00  movzbl 0x4024b0(%rdx),%edx      # 从数组中读取一个字节到 %edx 寄存器
  4010a0: 88 54 04 10           mov    %dl,0x10(%rsp,%rax,1)   # 将 %dl 寄存器的值移动到栈上的偏移量为当前索引的位置
  4010a4: 48 83 c0 01           add    $0x1,%rax                # 索引递增
  4010a8: 48 83 f8 06           cmp    $0x6,%rax                # 检查索引是否为6
  4010ac: 75 dd                 jne    40108b <phase_5+0x29>    # 如果不是，则继续循环
  4010ae: c6 44 24 16 00        movb   $0x0,0x16(%rsp)          # 在栈上的偏移量为22的位置存储0
  4010b3: be 5e 24 40 00        mov    $0x40245e,%esi           # 将数组的地址加载到 %esi 寄存器中
  4010b8: 48 8d 7c 24 10        lea    0x10(%rsp),%rdi          # 计算字符串的地址
  4010bd: e8 76 02 00 00        callq  401338 <strings_not_equal>  # 调用函数 strings_not_equal 比较两个字符串
  4010c2: 85 c0                 test   %eax,%eax                # 对 %eax 寄存器进行与运算，设置相关的标志位
  4010c4: 74 13                 je     4010d9 <phase_5+0x77>    # 如果两个字符串相等，则跳转到地址0x4010d9
  4010c6: e8 6f 03 00 00        callq  40143a <explode_bomb>     # 如果两个字符串不相等，则调用 explode_bomb 函数
  4010cb: 0f 1f 44 00 00        nopl   0x0(%rax,%rax,1)
  4010d0: eb 07                 jmp    4010d9 <phase_5+0x77>    # 跳转到地址0x4010d9，不执行后续指令
  4010d2: b8 00 00 00 00        mov    $0x0,%eax                # 将立即数0移动到 %eax 寄存器中
  4010d7: eb b2                 jmp    40108b <phase_5+0x29>    # 跳转到地址0x40108b，不执行后续指令
  4010d9: 48 8b 44 24 18        mov    0x18(%rsp),%rax          # 从栈上读取保存的栈底地址
  4010de: 64 48 33 04 25 28 00  xor    %fs:0x28,%rax             # 将栈底地址与当前线程的栈底地址进行异或操作
  4010e5: 00 00 
  4010e7: 74 05                 je     4010ee <phase_5+0x8c>    # 如果两者相等，则跳转到地址0x4010ee
  4010e9: e8 42 fa ff

```

​		phase_5相对复杂，考察的是类似于正则表达式的匹配机制，需要读取一个长度为6的字符串，对于每个字符截取后四位数字，用来作为index，获取另一个字符串里对应的字符，并保存起来，产生一个新的长度为6的字符串，要求等于另一个字符串。破局点在于发现 `0x4024b0` 和 `0x40245e` 这两个奇怪的非跳转地址，于是我们键入

```bash
$(gdb) print(char*) 0x4024b0
$(gdb) print (char*)0x40245e
```

​		可以看到，用来被截取的字符串str1为“maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?”，用来对比的字符串str2为“flyers”，我们发现flyers6个字符分别出现在str1的第9位，第15位，第14位，第5位，第6位，第7位，于是我们输入的字符串后四位的二进制只要分别表示9,15,14,5,6,7即可，笔者的答案为：

```
ionefg
```

##### Phase_6

```assembly
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14                      # 保存寄存器 %r14
  4010f6:	41 55                	push   %r13                      # 保存寄存器 %r13
  4010f8:	41 54                	push   %r12                      # 保存寄存器 %r12
  4010fa:	55                   	push   %rbp                      # 保存寄存器 %rbp
  4010fb:	53                   	push   %rbx                      # 保存寄存器 %rbx
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp                # 为局部变量分配80字节的栈空间
  401100:	49 89 e5             	mov    %rsp,%r13                # 将栈顶地址保存到 %r13 寄存器
  401103:	48 89 e6             	mov    %rsp,%rsi                # 将栈顶地址保存到 %rsi 寄存器
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>  # 调用函数 read_six_numbers 读入六个整数
  40110b:	49 89 e6             	mov    %rsp,%r14                # 将栈顶地址保存到 %r14 寄存器
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d               # 将0移动到 %r12d 寄存器
  401114:	4c 89 ed             	mov    %r13,%rbp                # 将栈顶地址保存到 %rbp 寄存器
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax           # 读取栈顶地址处的值到 %eax 寄存器
  40111b:	83 e8 01             	sub    $0x1,%eax                # 将 %eax 寄存器的值减1
  40111e:	83 f8 05             	cmp    $0x5,%eax                # 比较 %eax 寄存器的值是否小于等于5
  401121:	76 05                	jbe    401128 <phase_6+0x34>    # 如果是，则跳转到地址0x401128
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>     # 如果不是，则调用 explode_bomb 函数
  401128:	41 83 c4 01          	add    $0x1,%r12d               # %r12d 寄存器的值加1
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d               # 比较 %r12d 寄存器的值是否等于6
  401130:	74 21                	je     401153 <phase_6+0x5f>    # 如果是，则跳转到地址0x401153
  401132:	44 89 e3             	mov    %r12d,%ebx               # 将 %r12d 寄存器的值移动到 %ebx 寄存器
  401135:	48 63 c3             	movslq %ebx,%rax               # 将 %ebx 寄存器的值扩展为 %rax 寄存器
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax      # 从数组中读取一个整数到 %eax 寄存器
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)          # 比较 %eax 寄存器的值和栈上的值
  40113e:	75 05                	jne    401145 <phase_6+0x51>    # 如果不相等，则跳转到地址0x401145
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>     # 如果相等，则调用 explode_bomb 函数
  401145:	83 c3 01             	add    $0x1,%ebx                # %ebx 寄存器的值加1
  401148:	83 fb 05             	cmp    $0x5,%ebx                # 比较 %ebx 寄存器的值是否小于等于5
  40114b:	7e e8                	jle    401135 <phase_6+0x41>    # 如果是，则继续循环
  40114d:	49 83 c5 04          	add    $0x4,%r13                # 将栈顶地址后移4字节
  401151:	eb c1                	jmp    401114 <phase_6+0x20>    # 跳转到地址0x401114，继续循环
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi          # 计算 %rsi 寄存器的值
  401158:	4c 89 f0             	mov    %r14,%rax               # 将 %r14 寄存器的值移动到 %rax 寄存器
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx               # 将7移动到 %ecx 寄存器
  401160:	89 ca                	mov    %ecx,%edx               # 将 %ecx 寄存器的值移动到 %edx 寄存器
  401162:	2b 10                	sub    (%rax),%edx             # 将栈上的值减去 %edx 寄存器的值
  401164:	89 10                	mov    %edx,(%rax)             # 将 %edx 寄存器的值存入栈上的地址
  401166:	48 83 c0 04          	add    $0x4,%rax                # 栈上的地址后移4字节
  40116a:	48 39 f0             	cmp    %rsi,%rax               # 比较 %rsi 寄存器和 %rax 寄存器的值
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>    # 如果不相等，则继续循环
  40116f:	be 00 00 00 00       	mov    $0x0,%esi                # 将0移动到 %esi 寄存器
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>    # 跳转到地址0x401197
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx           # 从链表中取下一个结点
  40117a:	83 c0 01             	add    $0x1,%eax                # %eax 寄存器的值加1
  40117d:	39 c8                	cmp    %ecx,%eax               # 比较 %ecx 寄存器和 %eax 寄存器的值
  40117f:	75 f5                	jne    401176 <phase_6+0x82>    # 如果不相等，则继续循环
  401181:	eb 05                	jmp    401188 <phase_6+0x94>    # 跳转到地址0x401188
  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx           # 将数组的地址加载到 %edx 寄存器中
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)   # 将 %rdx 寄存器的值存入栈上的地址
  40118d:	48 83 c6 04          	add    $0x4,%rsi                # 栈上的地址后移4字节
  401191:	48 83 fe 18          	cmp    $0x18,%rsi               # 比较 %rsi 寄存器的值是否等于24
  401195:	74 14                	je     4011ab <phase_6+0xb7>    # 如果是，则跳转到地址0x4011ab
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx        # 从数组中读取一个整数到 %ecx 寄存器
  40119a:	83 f9 01             	cmp    $0x1,%ecx                # 比较 %ecx 寄存器的值是否等于1
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>    # 如果是，则继续循环
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax                # 将1移动到 %eax 寄存器
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx           # 将数组的地址加载到 %edx 寄存器中
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>    # 继续循环
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx          # 将栈上的值加载到 %rbx 寄存器中
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax         # 计算 %rax 寄存器的值
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi         # 计算 %rsi 寄存器的值
  4011ba:	48 89 d9             	mov    %rbx,%rcx               # 将 %rbx 寄存器的值移动到 %rcx 寄存器
  4011bd:	48 8b 10             	mov    (%rax),%rdx             # 读取栈上的值到 %rdx 寄存器
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)          # 将 %rdx 寄存器的值存入 %rcx+8 的位置
  4011c4:	48 83 c0 08          	add    $0x8,%rax                # 栈上的地址后移8字节
  4011c8:	48 39 f0             	cmp    %rsi,%rax               # 比较 %rs
  4011cb: 74 05                     je     4011d2 <phase_6+0xde>      ; Jump if the value in %rax is zero
  4011cd: 48 89 d1                  mov    %rdx,%rcx                 ; Move the value of %rdx into %rcx
  4011d0: eb eb                     jmp    4011bd <phase_6+0xc9>      ; Jump back to the beginning of the loop

  4011d2: 48 c7 42 08 00 00 00  	movq   $0x0,0x8(%rdx)             ; Set a 64-bit value at %rdx + 0x8 to zero
  4011d9: 00 
  4011da: bd 05 00 00 00        	mov    $0x5,%ebp                  ; Set up a counter (%ebp) for the loop (5 iterations)
  4011df: 48 8b 43 08           	mov    0x8(%rbx),%rax             ; Load a 64-bit value from %rbx + 0x8 into %rax
  4011e3: 8b 00                 	mov    (%rax),%eax               ; Load a 32-bit value from the memory location pointed to by %rax into %eax
  4011e5: 39 03                 	cmp    %eax,(%rbx)               ; Compare the value in %eax with the value at the memory location %rbx
  4011e7: 7d 05                 	jge    4011ee <phase_6+0xfa>      ; Jump to 4011ee if %eax is greater than or equal to the value at %rbx
  4011e9: e8 4c 02 00 00        	callq  40143a <explode_bomb>      ; Call explode_bomb if the comparison fails

  4011ee: 48 8b 5b 08           	mov    0x8(%rbx),%rbx             ; Move the next value at %rbx + 0x8 into %rbx
  4011f2: 83 ed 01              	sub    $0x1,%ebp                  ; Decrement the counter
  4011f5: 75 e8                 	jne    4011df <phase_6+0xeb>      ; Jump to 4011df if the counter is not zero

  4011f7: 48 83 c4 50           	add    $0x50,%rsp                 ; Clean up the stack
  4011fb: 5b                    	pop    %rbx                       ; Restore registers
  4011fc: 5d                    	pop    %rbp
  4011fd: 41 5c                 	pop    %r12
  4011ff: 41 5d                 	pop    %r13
  401201: 41 5e                 	pop    %r14
  401203: c3                    	retq                               ; Return from the function
```

​		phase_6是难度最大的一问，Writeup里也特意说明了 “hard for best students”。

​		大体上我么可以将它分成6个部分

1. 401106 读出6个int到rsp中
2. 40110e 检测六个int是否为[1,2,3,4,5,6]的任意排序
3. 401153 将这个六个int 变为 7 - int
4. 40116f 将第 7 - int个链表元素 放到 rsp0x20 + int 的相对位置 * 2的位置
5. 4011bd 链表，被换成了咱们数组中的顺序，和题目无关
6. 4011d2 检测没一个都curArrayValue >= nextArrayValue 即要求现在的数组降序

​		链表中的每个结点结构类似于：

```c
struct {
    int value;
    int order;
    node* next;
} node;
```

​		利用命令 `x/24 0x6032d0` 看可以查看链表的元素：

```bash
(gdb) x /d 0x6032d0
0x6032d0                     <node1> :            332

(gdb)x /d 0x6032e0 
0x6032e0                     <node2>:              168

(gdb) x /d 0x6032f0
0x6032f0                   <node3> :              924

(gdb) x /d 8x603300
0x603300                     <node4>:              691

(gdb) x /d 0x603310
                            0x603310 <node5> :     477
(gdb)x /d 0x603320 
                     0x603320 <node6> :            443
```

​		所以链表降序序列是：3 4 5 6 1 2

​		最后用7减去上面的序列之后就是：答案：4 3 2 1 6 5 

```text
4 3 2 1 6 5
```
