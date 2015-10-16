
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <modbus-tcp.h>
#include <modbus.h>
/*bacnet libraries*/
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

#define BACNET_INSTANCE_NO          12
#define BACNET_PORT                 0xBAC1
#define BACNET_INTERFACE            "lo"
#define BACNET_DATALINK_TYPE        "bvlc"
#define BACNET_SELECT_TIMEOUT_MS    1	/* ms */

#define RUN_AS_BBMD_CLIENT          1

#if RUN_AS_BBMD_CLIENT
#define BACNET_BBMD_PORT            0xBAC0
#define BACNET_BBMD_ADDRESS         "127.0.0.1"
#define BACNET_BBMD_TTL             90
#endif



#define SERVER "140.159.153.159"
#define PORT 502

#define NUM_LISTS 3
// linked list object
	typedef struct s_cur_val cur_val;
/*define structure containing a number and a pointer contatining the address of the next box*/
struct s_cur_val{
	int number;
	cur_val *next;
	};
// read register variables

int16_t tab_reg[64];
int rc;
int i;
//create a pointer to store current variables memory location
static cur_val *listhead[4];

#define NUM_TEST_DATA (sizeof(listhead)/sizeof(listhead[0]))
static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static int Update_Analog_Input_Read_Property(BACNET_READ_PROPERTY_DATA *
					     rpdata)
{
    static int index;
    int instance_no =
	bacnet_Analog_Input_Instance_To_Index(rpdata->object_instance);
    if (rpdata->object_property != bacnet_PROP_PRESENT_VALUE)
	goto not_pv;
    printf("AI_Present_Value request for instance %i\n", instance_no);
/* Update the values to be sent to the BACnet client here.
 * * The data should be read from the head of a linked list. You are required
 * * to implement this list functionality.
 * *
 * * bacnet_Analog_Input_Present_Value_Set()
 * * First argument: Instance No
 * * Second argument: data to be sent
 * *
 * * Without reconfiguring libbacnet, a maximum of 4 values may be sent */
    bacnet_Analog_Input_Present_Value_Set(0, listhead[0]->number);
  //  bacnet_Analog_Input_Present_Value_Set(1, listhead[1]); 
//    bacnet_Analog_Input_Present_Value_Set(2, listhead[2]);
    if (index == NUM_TEST_DATA)
	index = 0;
  not_pv:
    return bacnet_Analog_Input_Read_Property(rpdata);
}

static bacnet_object_functions_t server_objects[] = {
    {bacnet_OBJECT_DEVICE,
     NULL,
     bacnet_Device_Count,
     bacnet_Device_Index_To_Instance,
     bacnet_Device_Valid_Object_Instance_Number,
     bacnet_Device_Object_Name,
     bacnet_Device_Read_Property_Local,
     bacnet_Device_Write_Property_Local,
     bacnet_Device_Property_Lists,
     bacnet_DeviceGetRRInfo,
     NULL,			/* Iterator */
     NULL,			/* Value_Lists */
     NULL,			/* COV */
     NULL,			/* COV Clear */
     NULL			/* Intrinsic Reporting */
     },
    {bacnet_OBJECT_ANALOG_INPUT,
     bacnet_Analog_Input_Init,
     bacnet_Analog_Input_Count,
     bacnet_Analog_Input_Index_To_Instance,
     bacnet_Analog_Input_Valid_Instance,
     bacnet_Analog_Input_Object_Name,
     Update_Analog_Input_Read_Property,
     bacnet_Analog_Input_Write_Property,
     bacnet_Analog_Input_Property_Lists,
     NULL /* ReadRangeInfo */ ,
     NULL /* Iterator */ ,
     bacnet_Analog_Input_Encode_Value_List,
     bacnet_Analog_Input_Change_Of_Value,
     bacnet_Analog_Input_Change_Of_Value_Clear,
     bacnet_Analog_Input_Intrinsic_Reporting},
    {MAX_BACNET_OBJECT_TYPE}
};

static void register_with_bbmd(void)
{
#if RUN_AS_BBMD_CLIENT
/* Thread safety: Shares data with datalink_send_pdu */
    bacnet_bvlc_register_with_bbmd(bacnet_bip_getaddrbyname
				   (BACNET_BBMD_ADDRESS),
				   htons(BACNET_BBMD_PORT),
				   BACNET_BBMD_TTL);
#endif
}

static void *minute_tick(void *arg)
{
    while (1) {
	pthread_mutex_lock(&timer_lock);
/* Expire addresses once the TTL has expired */
	bacnet_address_cache_timer(60);
/* Re-register with BBMD once BBMD TTL has expired */
	register_with_bbmd();
/* Update addresses for notification class recipient list
 * * Requred for INTRINSIC_REPORTING
 * * bacnet_Notification_Class_find_recipient(); */
/* Sleep for 1 minute */
	pthread_mutex_unlock(&timer_lock);
	sleep(60);
    }
    return arg;
}

static void *second_tick(void *arg)
{
    while (1) {
	pthread_mutex_lock(&timer_lock);
/* Invalidates stale BBMD foreign device table entries */
	bacnet_bvlc_maintenance_timer(1);
/* Transaction state machine: Responsible for retransmissions and ack
 * * checking for confirmed services */
	bacnet_tsm_timer_milliseconds(1000);
/* Re-enables communications after DCC_Time_Duration_Seconds
 * * Required for SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL
 * * bacnet_dcc_timer_seconds(1); */
/* State machine for load control object
 * * Required for OBJECT_LOAD_CONTROL
 * * bacnet_Load_Control_State_Machine_Handler(); */
/* Expires any COV subscribers that have finite lifetimes
 * * Required for SERVICE_CONFIRMED_SUBSCRIBE_COV
 * * bacnet_handler_cov_timer_seconds(1); */
/* Monitor Trend Log uLogIntervals and fetch properties
 * * Required for OBJECT_TRENDLOG
 * * bacnet_trend_log_timer(1); */
/* Run [Object_Type]_Intrinsic_Reporting() for all objects in device
 * * Required for INTRINSIC_REPORTING
 * * bacnet_Device_local_reporting(); */
/* Sleep for 1 second */
	pthread_mutex_unlock(&timer_lock);
	sleep(1);
    }
    return arg;
}

static void ms_tick(void)
{
/* Updates change of value COV subscribers.
 * * Required for SERVICE_CONFIRMED_SUBSCRIBE_COV
 * * bacnet_handler_cov_task(); */
}

#define BN_UNC(service, handler) \
bacnet_apdu_set_unconfirmed_handler( \
SERVICE_UNCONFIRMED_##service, \
bacnet_handler_##handler)
#define BN_CON(service, handler) \
bacnet_apdu_set_confirmed_handler( \
SERVICE_CONFIRMED_##service, \
bacnet_handler_##handler)

/*linked list object */
typedef struct s_word_object word_object;
struct s_word_object {
    char *word;
    word_object *next;
};

static word_object *list_heads[NUM_LISTS];
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t list_data_ready = PTHREAD_COND_INITIALIZER;
static pthread_cond_t list_data_flush = PTHREAD_COND_INITIALIZER;

/* add object to list*/
static void add_to_list(word_object ** list_head, char *word)
{
    word_object *last_object, *tmp_object;
    char *tmp_string;

    //do all allocation outside of locking
    tmp_object = malloc(sizeof(word_object));
    tmp_string = strdup(word);

    //setup tmp_object outside of locking
    tmp_object->word = tmp_string;
    tmp_object->next = NULL;

    pthread_mutex_lock(&list_lock);



    if (*list_head == NULL) {
	*list_head = tmp_object;
    } else {
	last_object = *list_head;
	while (last_object->next) {
	    last_object = last_object->next;
	}
	last_object->next = tmp_object;

    }
    pthread_mutex_unlock(&list_lock);
    pthread_cond_signal(&list_data_ready);
}

static word_object *list_get_first(word_object ** list_head)
{
    word_object *first_object;

    first_object = *list_head;
    *list_head = (*list_head)->next;

    return first_object;
}

static void *print_func(void *arg)
{
    word_object **list_head = (word_object **) arg;

    word_object *current_object;

    fprintf(stderr, "Print thread starting\n");
    while (1) {
	pthread_mutex_lock(&list_lock);
	while (*list_head == NULL) {
	    pthread_cond_wait(&list_data_ready, &list_lock);
	}
	current_object = list_get_first(list_head);
	pthread_mutex_unlock(&list_lock);
/* printf() and free() can block, make sure that we've released
 * * list_lock first */
	printf("Print thread: %s\n", current_object->word);
	free(current_object->word);
	free(current_object);
/* Let list_flush() know that we've done some work */
	pthread_cond_signal(&list_data_flush);
    }
/* Silence compiler warning */
    return arg;
}

static void list_flush(word_object * list_head)
{
    pthread_mutex_lock(&list_lock);
    while (list_head != NULL) {
	pthread_cond_signal(&list_data_ready);
	pthread_cond_wait(&list_data_flush, &list_lock);
    }
    pthread_mutex_unlock(&list_lock);
}


static int modbusrun(void)
{

/* Initialise Modbus*/
    modbus_t *ctx;
/* Modbus Client*/
    ctx = modbus_new_tcp(SERVER, PORT);	/*set IP address and port of server */

    if (ctx == NULL) {
	fprintf(stderr, "Unable to allocate libmodbus context\n");
	return -1;
    } else {
	fprintf(stderr, "Modbus created succesfully\n");
    }

    /*Connect to the modbus Server */
    if (modbus_connect(ctx) == -1) {
	fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
	modbus_free(ctx);	/* Free modbus_t structure */
	return -1;
    }

    else {
	fprintf(stderr, "connection successful!\n");
    }
    printf("starting loop\n");
    while (1) {

	/*read the registers on the server */
	rc = modbus_read_registers(ctx, 12, 3, tab_reg);
	if (rc == -1) {
	    fprintf(stderr, "%s\n", modbus_strerror(errno));
	    return -1;
	}

	for (i = 0; i < rc; i++)
	    printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);
	usleep(100000);

    }

    modbus_close(ctx);
    /* Close modbus connection */
    modbus_free(ctx);
    /* free modbus */



}






int main(void)
{
    modbusrun();
}
