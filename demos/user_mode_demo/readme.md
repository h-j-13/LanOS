# 血与泪

### 记录一下刚刚调试的要点

最初崩溃现象如下
	
	在调用了一段时间的系统调用之后，莫名随机崩溃
	发现栈一致在涨，溢出了
	发现默认中断处理函数ignore_int一直被调用
	关闭时钟终端之后崩溃消失
	猜测应该是cpu报错
	实现cpu报错的0-16错误函数
	发现报错在13号终端，也就是general_protection
	在时钟中断下断点，发现如果在系统调用内核态时发生时钟中断
	由于要使用out指令，需要临时用al寄存器，但是并没有正确恢复
	在返回到系统调用代码时，eax被当作系统调用号，原本是0
	这里被改成0x20
	所以造成访问函数列表数组越界

解决方案

	在timer_interrupt中push eax 最后pop eax，保证eax的正确即可