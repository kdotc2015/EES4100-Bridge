#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <modbus-tcp.h>
#include <modbus.h>


#define SERVER "140.159.153.159"
#define PORT 502

modbus_t *ctx;
// read register variables
int16_t tab_reg[64];
int rc;
int i;


/*linked list object */
typedef struct s_word_object word_object;


struct s_word_object {
    char *word;
    word_object *next;
};

static word_object *list_head;

/* add object to list*/
static void add_to_list(char *word)
{
    word_object *last_object;

    if (list_head == NULL) {
	last_object = malloc(sizeof(word_object));	//allocate enough memory
	list_head = last_object;
    } else {
	last_object = list_head;
	while (last_object->next) {
	    last_object = last_object->next;
	}
	last_object->next = malloc(sizeof(word_object));

	last_object = last_object->next;

    }
    last_object->word = strdup(word);
    last_object->next = NULL;
}


static int modbusinit(void)
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

    /*read the registers on the server */
    rc = modbus_read_registers(ctx, 0, 3, tab_reg);
    if (rc == -1) {
	fprintf(stderr, "%s\n", modbus_strerror(errno));
	return -1;
    }

    for (i = 0; i < rc; i++)
	printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);
}



modbus_close(ctx);
		/* Close modbus connection */
modbus_free(ctx);
		/* free allocated modbus_t structure */







int main(void)
{
    modbusinit();
    while (1) {
	/*read the registers on the server */
	rc = modbus_read_registers(ctx, 0, 3, tab_reg);
	if (rc == -1) {
	    fprintf(stderr, "%s\n", modbus_strerror(errno));
	    return -1;
	}


	for (i = 0; i < rc; i++)
	    printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);

    }
}
