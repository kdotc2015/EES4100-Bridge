#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <modbus-tcp.h>
#include <modbus.h>

/* BACnet libraries*/
#include <netinet/in.h>
#include <libbacnet/address.h>
#include <libbacnet/device.h>
#include <libbacnet/handlers.h>
#include <libbacnet/datalink.h>
#include <libbacnet/bvlc.h>
#include <libbacnet/client.h>
#include <libbacnet/txbuf.h>
#include <libbacnet/tsm.h>
#include <libbacnet/ai.h>
#include "bacnet_namespace.h"

#define SERVER "140.159.153.159"
#define PORT 502

#define BACNET_INSTANCE_NO	120
#define BACNET_PORT	0xBAC1
#define BACNET_INTERFACE	"lo"
#define BACNET_DATALINK_TYPE	"bvlc"
#define BACNET_SELECT_TIMEOUT_MS 1	/* ms*/

#define RUN_AS_BBMD_CLIENT	1
#if RUN_AS_BBMD_CLIENT
#define BACNET_BBMD_PORT	0xBAC0
#define BACNET_BBMD_ADDRESS	"127.0.0.1"
#define BACNET_BBMD_TTL	90
#endif

int16_t tab_reg[64];
int rc;
int i;

static pthread_mutex_t timerlock=PTHREAD_MUTEX_INITIALIZER;

static bacnet_object_functions_t serverobj[]={
	{bacnet_OBJECT_DEVICE,
		NULL,
		bacnet_Device_Count,
		bacnet_Device_Valid_Object_Instance_Number,
		bacnet_Device_Object_Name,
		bacnet_Device_Read_Property_Local,
		bacnet_Device_Write_Property_Local,
		bacnet_Device_Property_Lists,
		bacnet_DeviceGetRRInfo,
		NULL, /* Iterator */
		NULL, /* Value_Lists */
		NULL, /* COV */
		NULL, /* COV Clear */
		NULL /* Intrinsic Reporting */
	},


	{bacnet_OBJECT_ANALOG_INPUT,
		bacnet_Analog_Input_Init,
		bacnet_Analog_Input_Count,
		bacnet_Analog_Input_Index_To_Instance,
		bacnet_Analog_Input_Valid_Instance,
		bacnet_Analog_Input_Object_Name,
		bacnet_Analog_Input_Read_Property,
		bacnet_Analog_Input_Write_Property,
		bacnet_Analog_Input_Property_Lists,
		NULL /* ReadRangeInfo */ ,
		NULL /* Iterator */ ,
		bacnet_Analog_Input_Encode_Value_List,
		bacnet_Analog_Input_Change_Of_Value,
		bacnet_Analog_Input_Change_Of_Value_Clear,
		bacnet_Analog_Input_Intrinsic_Reporting
	},
		
	{MAX_BACNET_OBJECT_TYPE}
}

static void bbmd_register(void){
#if RUN_AS_BBMD_CLIENT

	bacnet_bvlc_register_with_bbmd(
		bacnet_bip_getaddrbyname(BACNET_BBMD_ADDRESS),
		htons(BACNET_BBMD_PORT),
		BACNET_BBMD_TTL),

#endif
}
static int modbusinit(void){

/* Initialise Modbus*/
modbus_t *ctx;
/* Modbus Client*/
ctx = modbus_new_tcp(SERVER, PORT);/*set IP address and port of server*/

        if (ctx == NULL){
	        fprintf(stderr, "Unable to allocate libmodbus context\n");
		        return -1;
			}
	else{		                
		fprintf(stderr,"Modbus created succesfully\n");
		}

	 /*Connect to the modbus Server*/
	 if (modbus_connect(ctx) == -1) {
		fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
		modbus_free(ctx);/* Free modbus_t structure */
	        return -1;														             }
																                  else{															                  fprintf(stderr,"connection successful!\n");
																	               }														               /*read the registers on the server*/
																		       rc = modbus_read_registers(ctx, 0, 3, tab_reg);
																			if (rc == -1) {
				fprintf(stderr, "%s\n", modbus_strerror(errno));
	                        return -1;																											                           }

			for (i=0; i < rc; i++) 															 printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]); 
			}																																			                           
															                           modbus_close(ctx);/* Close modbus connection*/																																										                     modbus_free(ctx);/* free allocated modbus_t structure*/
   
}




int main(void){
modbusinit();
while(1){
	/*read the registers on the server*/
                 rc = modbus_read_registers(ctx, 0, 3, tab_reg);
		                   if (rc == -1) {
	                 			fprintf(stderr, "%s\n", modbus_strerror(errno));		                                   				      return -1;                                                                                                                               }
						
				    for (i=0; i < rc; i++)                                                                                                                   printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);
															              		}
}
