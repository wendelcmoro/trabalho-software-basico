#include "../include/cMIPS.h"
#include "../include/uart_defs.h"

#define SPEED 5

//retorna nrx
int proberx(void){
	extern UARTdriver volatile Ud;
	return (Ud.nrx);
}

//retorna ntx
int propetx(void){
	extern UARTdriver volatile Ud;
	return (Ud.ntx);
}

//retorna inteiro com status no byte menos sign.
Tstatus iostat(void){
	Tserial volatile *uart;
	uart= (void *)IO_UART_ADDR;

	return uart->stat;
}

//escreve byte menos sign no reg. de controle
void ioctl(Tcontrol ctrl){
	Tserial volatile *uart;
	uart= (void *)IO_UART_ADDR;

	uart->ctl = ctrl;
}


//escreve byte menos sign no reg. de interrupções
int ioint(Tinterr interr){
    Tserial volatile *uart;
    uart= (void *)IO_UART_ADDR;
	
    int old_value = uart->interr.i;
    uart->interr.i = uart->interr.i | interr.i;
	
    return old_value;
}

//retira o octeto da fila, decrementa nrx
char Getc(void){
	extern UARTdriver volatile Ud;
	disableInterr();

	char aux = Ud.rx_q[Ud.rx_hd];
	Ud.nrx--;
	Ud.rx_hd=(Ud.rx_hd + 1) % Q_SZ;

	enableInterr();
	return aux;
}

//insere octeto na fila, decrementa ntx
void Putc(char c){
	extern UARTdriver volatile Ud;
	disableInterr();

  	Ud.ntx--;
  	Ud.tx_q[Ud.tx_tl] = c;
  	Ud.tx_tl = (Ud.tx_tl + 1) % Q_SZ;

  	if ((propetx() < 16) && iostat().txEmpty == 1) {
  	    Tinterr interr;
  	    interr.i = UART_INT_setTX | UART_INT_progTX;
  	    ioint(interr);
  	}

  	enableInterr();
}

//Inicializa a UART
void InitUART(void){
	extern UARTdriver volatile Ud;

    // reset all UART's signals
	Tcontrol ctrl;
	ctrl.ign = 0;
	ctrl.rts = 0;      // make RTS=0 to keep RemoteUnit inactive
	ctrl.ign4 = 0;
	ctrl.speed = SPEED;  // operate at the second highest data rate
	ioctl(ctrl);

	Tinterr interr;
	interr.i = 0;
	ioint(interr);

    interr.i = UART_INT_progRX;
    ioint(interr);

// Inicializa variaveis de transmissao
	Ud.ntx = 16;
	Ud.tx_hd = 0;
	Ud.tx_tl = 0;

// Inicializa variaveis de recepcao
	Ud.nrx = 0;
	Ud.rx_hd = 0;
	Ud.rx_tl = 0;

    ctrl.ign = 0;
    ctrl.rts = 1;      // make RTS=1 to activate remote unit
    ctrl.ign4 = 0;
    ctrl.speed = SPEED;  // operate at the second highest data rate
    ioctl(ctrl);
}

int get_primo(char *reception_queue, int *r_hd) {
    static int primos[50] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89,
                             97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
                             191, 193, 197, 199, 211, 223, 227, 229};
    int primo;
    if (reception_queue[*r_hd] >= 48) {
        int index = 0;
        while (reception_queue[*r_hd] >= 48) {    // enquanto for um número
            int mult = (reception_queue[*r_hd + 1] < 48) ? 1 : 10;  // Verifica se é decimal ou unitário
            index += ((int) reception_queue[*r_hd] - 48) * mult;
            *r_hd = (*r_hd + 1) % 256;
        }
        primo = primos[index];
    } else {
        primo = -1;
    }
    *r_hd = (*r_hd + 1) % 256;
    return primo;
}

int convert_primo(char *transmission_queue, int *tx_tl, int primo) {
    while (primo > 0) {
        int div;
        if (primo > 10) {
            div = (primo > 100) ? 100 : 10;
        }
        else {
            div = 1;
        }
        int number_int = primo / div;
        primo %= div;
        transmission_queue[*tx_tl] = number_int + 48;       // Convert number in int to char
        *tx_tl = (*tx_tl + 1) % 512;
    }
    transmission_queue[*tx_tl] = '\n';
    *tx_tl = (*tx_tl + 1) % 512;
    return 0;
}

int main(void) {
    InitUART();
    enableInterr();

    char c = 'a';
    char reception_queue[256];
    int r_tl = 0;
    int r_hd = 0;
    char transmission_queue[512];
    int tx_tl = 0;
    int tx_hd = 0;
    int primo = 0;
    do {
        while (c != EOT) {  // check if there is a new char.
            if (proberx() > 0) {
                c = Getc();
                reception_queue[r_tl] = c;
                r_tl = (r_tl + 1) % 256;
                if (c == '\n' && proberx() < 5)
                    break;
            }
        }
        while (proberx() < 5 && r_hd != r_tl) {
            if (!primo)
                primo = get_primo(reception_queue, &r_hd);
            if (proberx() < 5 && primo) {
                primo = convert_primo(transmission_queue, &tx_tl, primo);
            }
            while (proberx() < 5 && tx_hd != tx_tl) {
                if (propetx() > 0) {
                    Putc(transmission_queue[tx_hd]); //put the correspondent number in tx queue
                    tx_hd = (tx_hd + 1) % 512;
                }
            }
        }
    } while (c != EOT);

    unsigned int volatile delay = 0;
    while (propetx() < 16) {
        delay_us(100);
        delay++;
    }
    return delay;
}
