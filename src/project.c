#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
//#include <modbus_tcp.h>
#include <modbus.h>

#define SERVER "140.159.153.159"
#define PORT "502"

unint16_t tab_reg[64];
int rc;
int i;

int main(void){
// Initialise Modbus 
modbus_t *ctx;

ctx = modbus_new_tcp(SERVER, PORT);/*set IP address and port of server*/
	
	if (ctx == NULL){	          
	fprintf(stderr, "Unable to allocate libmodbus context\n");
	return -1;
	
	}
	else{
		fprintf(stderr,"Modbus created succesfully\n");
	}

	 //Connect to the modbus Server
	if (modbus_connect(ctx) == -1) {
	     fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	     modbus_free(ctx);/* Free modbus_t structure */
	     return -1;
	     }
	     else{
		fprintf(stderr,"connection successful!\n");
	     }
	//read the registers on the server
	rc = modbus_read_registers(ctx, 0, 10, tab_reg);
		if (rc == -1) {
		    	fprintf(stderr, "%s\n", modbus_strerror(errno));
			return -1;
		    }

		for (i=0; i < rc; i++) {
			printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);				 }
			
			modbus_close(ctx);/* Close modbus connection*/
			modbus_free(ctx);/* free allocated modbus_t structure*/

}
