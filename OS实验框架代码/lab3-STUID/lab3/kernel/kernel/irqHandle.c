#include "x86.h"
#include "device.h"

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case 0:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case 1:
			syscallFork(tf);
			break; // for SYS_FORK
		case 2:
			syscallExec(tf);
			break; // for SYS_EXEC
		case 3:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case 4:
			syscallExit(tf);
			break; // for SYS_EXIT
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	// TODO in lab3
	int minrunnable = MAX_PCB_NUM;  // the runnable user pcb whose index is min and not IDLE pcb
	int i=0;
    // find blocked pcbs, whose sleeptime--, 
    // and when sleeptime = 0, turn to runnable pcb
	for(i=0; i<MAX_PCB_NUM;i++)
	{
	    
		if (pcb[i].state == STATE_BLOCKED) 
		{
			pcb[i].sleepTime--;
			if(pcb[i].sleepTime == 0)
			{
				pcb[i].state = STATE_RUNNABLE;
			}
		}
	}
	// current pcb, whose timeCount++
	// and when time over, switch:
	// let minrunnable user pcb be running if exists
	// or let IDLE be running if it's runnable
	// otherwise, let current pcb be running again
	pcb[current].timeCount++;
	if(pcb[current].timeCount == MAX_TIME_COUNT) 
	{
		      // find next runnable user pcb after currentpcb
			 for(i=current;i<MAX_PCB_NUM;i++)
	         {
		        if (pcb[i].state == STATE_RUNNABLE) 
		        {
			        if (i < minrunnable){
				         minrunnable = i;
			        }
		        }
	         }
			 // find next runnable user pcb from beginning
			 if(minrunnable == MAX_PCB_NUM)
			 {
				 for(i=1;i<current;i++)
	             {
		            if (pcb[i].state == STATE_RUNNABLE) 
		            {
			             if (i < minrunnable){
				            minrunnable = i;
			            }
		            }
	             }
			 }
             // minrunnable user pcb exists
		     if(minrunnable != MAX_PCB_NUM)  
		     {
			     pcb[minrunnable].state = STATE_RUNNING;
			     pcb[minrunnable].timeCount = 0;
			     pcb[current].state = STATE_RUNNABLE;
				 current = minrunnable;
		     } 
			 // only IDLE runnable
		     else if(pcb[0].state == STATE_RUNNABLE) 
		     {
			     pcb[0].state = STATE_RUNNING;
			     pcb[0].timeCount = 0;
			     pcb[current].state = STATE_RUNNABLE;
				 current = 0;
		     }
			 // no pcb runnable
		     else  
		     {
			    pcb[current].timeCount = 0;
		     }	

			 // switch process's stack and registers
		     uint32_t tmpStackTop = pcb[current].stackTop;
             pcb[current].stackTop = pcb[current].prevStackTop;
             tss.esp0 = (uint32_t)&(pcb[current].stackTop);  // set tss.esp0 where has kernel stack
             asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
             asm volatile("popl %gs");
             asm volatile("popl %fs");
             asm volatile("popl %es");
             asm volatile("popl %ds");
             asm volatile("popal");
             asm volatile("addl $8, %esp");
             asm volatile("iret");
	}
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	putChar(getChar(keyCode));
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case 0:
			syscallPrint(tf);
			break; // for STD_OUT
		default:break;
	}
}

void syscallPrint(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

void syscallFork(struct TrapFrame *tf) {
	// TODO in lab3
	int has = 0;
	int i= 0; int j=0;
	int childpcb = current;
	for(i=1;i<MAX_PCB_NUM&&i!=current;i++)
	{
		if(pcb[i].state == STATE_DEAD)  // to find one empty pcb
		{
			has = 1;
			childpcb = i;
			break;
		}
	}


	if(has == 0)  //no empty, fork failed, 
	{
		pcb[current].regs.eax = -1;                      // father return -1
	}
	else // found one, fork successed,
	{
		pcb[current].regs.eax = childpcb;               // father return childpid

		// memeory copy
		for (j = 0; j < 0x100000; j++) 
		{
			*(uint8_t *)(j + (childpcb + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) *0x100000);
        }
		// pcb copy, which is none of father's business
	    pcb[childpcb].stackTop = (uint32_t)&(pcb[childpcb].regs);
	    pcb[childpcb].prevStackTop = (uint32_t)&(pcb[childpcb].stackTop);

	    pcb[childpcb].state = STATE_RUNNABLE; 
	    pcb[childpcb].timeCount = 0;  
	    pcb[childpcb].sleepTime = 0; 
	    pcb[childpcb].pid = childpcb; 

		pcb[childpcb].regs.eax = 0;                      // child return 0 
		// TrapFrame copy, which should be worked out
		pcb[childpcb].regs.cs = USEL(childpcb*2+1);
	    pcb[childpcb].regs.ss = USEL(childpcb*2+2);
	    pcb[childpcb].regs.ds = USEL(childpcb*2+2);
	    pcb[childpcb].regs.es = USEL(childpcb*2+2);
	    pcb[childpcb].regs.fs = USEL(childpcb*2+2);
	    pcb[childpcb].regs.gs = USEL(childpcb*2+2);
		// TrapFrame copy, which can be inherited from father
		pcb[childpcb].regs.esp = pcb[current].regs.esp;
		pcb[childpcb].regs.ebp = pcb[current].regs.ebp;
		pcb[childpcb].regs.edi = pcb[current].regs.edi;
		pcb[childpcb].regs.esi = pcb[current].regs.esi;

		pcb[childpcb].regs.eflags = pcb[current].regs.eflags;
		pcb[childpcb].regs.eip = pcb[current].regs.eip;

		pcb[childpcb].regs.ebx = pcb[current].regs.ebx;
		pcb[childpcb].regs.ecx = pcb[current].regs.ecx;
		pcb[childpcb].regs.edx = pcb[current].regs.edx;

		pcb[childpcb].regs.error = pcb[current].regs.error;
		pcb[childpcb].regs.irq = pcb[current].regs.irq;
		pcb[childpcb].regs.xxx = pcb[current].regs.xxx;
	}
	
	return;
}

void syscallExec(struct TrapFrame *tf) {
	// TODO in lab3
	// read filename
	int sel = tf->ds; 
	char *str = (char *)tf->ecx;  // str: filename
	char character[200];  // filename copy
	uint32_t count = 0;
	int i=0; 
	int ret = -1;
	uint32_t entry = 0;

	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < 200; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character[count]):"r"(str + i));
		if (character[count] == '\n') 
		{
			count--;
		}
		if(character[count] == '\0')
		{
			count++;
			break;
		}
		count++;
	}

	// loadElf
    ret = loadElf(character,(current + 1) * 0x100000, &entry);

	// return
	if(ret == -1)  // load failed
	{
		tf->eax = -1;  // return -1
	}
	else if(ret == 0) // load succeeded
	{
		tf->eip = entry;  // no return
	}
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3
	if(tf->ecx > 0)
	{
		pcb[current].state = STATE_BLOCKED;  // state -> blocked
		pcb[current].sleepTime = tf->ecx; // ecx = time (from syscall.c/sleep(time))
	    asm volatile("int $0x20");  // call timerHandle()
	}
	return;
}

void syscallExit(struct TrapFrame *tf) {
	// TODO in lab3
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
