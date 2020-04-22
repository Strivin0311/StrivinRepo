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
	int currentpcb = MAX_PCB_NUM;
	int minrunnable = MAX_PCB_NUM;  // the runnable user pcb whose index is min and not IDLE pcb

	for(int i=0; i<MAX_PCB_NUM;i++)
	{
	    // find blocked pcbs, whose sleeptime--, 
		// and when sleeptime = 0, turn to runnable pcb
		if (pcb[i].state == STATE_BLOCKED) 
		{
			pcb[i].sleepTime--;
			if(pcb[i].sleepTime <= 0)
			{
				pcb[i].state = STATE_RUNNABLE;
				if (i < minrunnable && i != 0){
				   minrunnable = i;
			     }
			}
		}
		// runnable pcbs
		else if (pcb[i].state == STATE_RUNNABLE)
		{
			if (i < minrunnable && i != 0){
				   minrunnable = i;
			     }
		}
		// get running/current pcb 
		else if(pcb[i].state == STATE_RUNNING)
		{
			currentpcb = i;
		}
	}

	// current pcb, whose timeCount++
	// and when timeCount over, let minrunnable user pcb be running if exists
	// or let IDLE be running if it's runnable
	// otherwise, let current pcb be running again
	if(currentpcb!=MAX_PCB_NUM)
	{
		pcb[currentpcb].timeCount++;
	    if(pcb[currentpcb].timeCount == MAX_TIME_COUNT)
	    {
		     if(minrunnable != MAX_PCB_NUM)  // minrunnable user pcb exists
		     {
			     pcb[minrunnable].state = STATE_RUNNING;
			     pcb[minrunnable].timeCount = 0;
			     pcb[currentpcb].state = STATE_RUNNABLE;
		     } 
		     else if(pcb[0].state == STATE_RUNNABLE) // only IDLE runnable
		     {
			     pcb[0].state = STATE_RUNNING;
			     pcb[0].timeCount = 0;
			     pcb[currentpcb].state = STATE_RUNNABLE;
		     }
		     else  // no pcb runnable
		     {
			    pcb[currentpcb].timeCount = 0;
		     }	
	    }
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

	return;
}

void syscallExec(struct TrapFrame *tf) {
	// TODO in lab3
	// hint: ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	// TODO in lab3
	return;
}

void syscallExit(struct TrapFrame *tf) {
	// TODO in lab3
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}
