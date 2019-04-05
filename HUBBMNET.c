#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//structures for all frame layers

typedef struct{
    char sender_mac[11];
    char receiver_mac[11];
} Physical;

typedef struct{
    char sender_ip[8];
    char receiver_ip[8];
} Network;

typedef struct{
    char sender_port[5];
    char receiver_port[5];
} Transport;

typedef struct{
    char sender_id[2];
    char receiver_id[2];
    char message[10000];
} Application;

//frame structure for getting all layers into one type array
typedef struct{
    char *flag;
    Physical phy;
    Network netw;
    Transport tport;
    Application app;
} Frame;

//structures for arrays (queue,routing..)
typedef struct{
    char to_send[2];
    char by_using[2];
} Routing;

typedef struct{ //Struct for client properties
    char client_id[2];
    char client_ip[8];
    char client_mac[11];
} Clients;

typedef struct{
    char sender_id[2];
    char receiver_id[2];
    char timme[50];
    char date[50];
    char message[10000];
    int total_frames;
    int total_hops;
    char activity_type[20];
    char success_status[10];
} Log;

typedef struct{
    int entry_size;
    Log *log_str;
} LogArray;

typedef struct{
    int front_incoming;
    int front_outgoing;
    int rear_incoming;
    int rear_outgoing;
    Frame **incoming_queue;
    Frame **outgoing_queue;
    int outsize;
    int insize;
} Queue;

//Global Variables
int client_size;
Clients *client;
Routing **route;
Frame frames[4];
Queue *queue;
LogArray *logs;
int top = -1; //stack
int hop = 0;

/*
*readClientsRouting function reads two files named clients and routing.
*function puts that data gets from files into clients array and routing array.
*also, it creates dynamic queue and logs arrays.
*/

void readClientsRouting(FILE *clientFile, FILE *routingFile){ //Reads client file and put properties to Client Structure
    int i,j;
    char x[2];

    fscanf(clientFile, "%d", &client_size); //gets client size
    client = malloc(client_size * sizeof(Clients)); //allocating client array depending on client size

    for (i = 0; i < client_size; i++){
        fscanf(clientFile, "%s %s %s", client[i].client_id, client[i].client_ip, client[i].client_mac); //put them into array
    }

    route = malloc(client_size * sizeof(Routing*)); //allocating twodimensional route array depending on client size

    for (i = 0; i < client_size; i++){
        route[i] = malloc((client_size - 1) * sizeof(Routing));  //allocate two dimensional array
    }

    for (i = 0; i < client_size; i++){
        for (j=0; j < client_size - 1; j++){
            fscanf(routingFile, "%s %s", route[i][j].to_send, route[i][j].by_using); 
        }
        fscanf(routingFile, "%s", &x); //it passes "-" between two client information
    }

    queue = malloc(client_size * sizeof(Queue));
    logs = malloc(client_size * sizeof(LogArray));

    //initializing some data depending on queue basic
    for (i = 0; i < client_size ; i++){
        queue[i].front_incoming = 0;
        queue[i].front_outgoing = 0;
        queue[i].rear_incoming = -1;
        queue[i].rear_outgoing = -1;
        queue[i].insize = 0; //it holds how many frame is in incoming queue
        queue[i].outsize = 0; //it holds how many frame is in outgoing queue
        logs[i].entry_size = 0;
    }
}

void push(Frame fr){ //push function for frame stack
    top += 1;
    frames[top] = fr;
}

void pop(){ //pop function for frame stack and it prints what function popped
    Frame fr;
    fr = frames[top];

    if (strcmp(fr.flag, "Physical") == 0) //checking what frame is
        printf("Sender MAC address: %s, Receiver MAC address: %s\n", fr.phy.sender_mac, fr.phy.receiver_mac);

    else if (strcmp(fr.flag, "Network") == 0)
        printf("Sender IP address: %s, Receiver IP address: %s\n", fr.netw.sender_ip, fr.netw.receiver_ip);

    else if (strcmp(fr.flag, "Transport") == 0)
        printf("Sender port number: %s, Receiver port number: %s\n", fr.tport.sender_port, fr.tport.receiver_port);
    
    else{
        printf("Sender ID: %s, Receiver ID: %s\nMessage chunk carried: %s\n", fr.app.sender_id, fr.app.receiver_id, fr.app.message);
        printf("--------\n");
    }    

    top = top - 1;
}

void insert_income(int j, Frame fr[]){ //inserting data into incoming queue of location j (j is which client's location)
    int i;
    queue[j].rear_incoming++;
    queue[j].insize++;

    for (i=0; i < 4; i++){
        queue[j].incoming_queue[queue[j].rear_incoming][i] = fr[i];
    }
}

void insert_outcome(int j, Frame fr[]){ //inserting data into outgoing queue of location j (j is which client's location)
    int i;
    queue[j].rear_outgoing++;
    queue[j].outsize++;

    for (i = 0; i < 4 ; i++){
        queue[j].outgoing_queue[queue[j].rear_outgoing][i] = fr[i];
    }
}

Frame* peek_outcome(int location){ //returns the first data in outcoming queue
    return queue[location].outgoing_queue[queue[location].front_outgoing];
}

Frame* peek_income(int location){ //returns the first data in incoming queue
    return queue[location].incoming_queue[queue[location].front_incoming];
}

void remove_income(int j){ //removes the first data in incoming queue
    free(queue[j].incoming_queue[queue[j].front_incoming++]);
    queue[j].insize--;
    queue[j].rear_incoming--;
}

void remove_outcome(int j){ //removes the first data in outgoing queue
    free(queue[j].outgoing_queue[queue[j].front_outgoing++]);
    queue[j].outsize--;
    queue[j].rear_outgoing--;
}

/*
*routeControl function controls that, if a client wants to reach another client, 
*checks which client it should go first to reach wanted one.
*it returns the location of which client used to reach wanted one.
*if there is not a way to reach wanted one, then returns -1.
*/

int routeControl(int location_receiver, int location_sender){
    int location_by_using, i,j;

    for (i = 0; i < client_size - 1; i++){
        if (strcmp(route[location_sender][i].to_send, client[location_receiver].client_id) == 0){
            location_by_using = i;
            route[location_sender][location_by_using].by_using;

            for (j=0; j<client_size;j++){
                if (strcmp(client[j].client_id, route[location_sender][location_by_using].by_using) == 0){
                    location_by_using = j;
                }
            }
            return location_by_using;
        }
    }
    return -1;
}

/*
*commandessage function first initialize the frame size depending on message length and maximum message size.
*then it starts to generate frame.
*copies datas into frame.
*after finishing a frame, it allocates memory on sender's outgoing queue and puts frame into that queue.
*lastly, it pops all data from frame.
*/

void commandMessage(char* sender_id, char* receiver_id, char* message, int msg_size, char* incoming_port, char* outgoing_port){
    printf("---------------------------------------------------------------------------------------\n");
    printf("%s: %s %s %s #%s#\n", "Command", "MESSAGE", sender_id, receiver_id, message);
    printf("---------------------------------------------------------------------------------------\n");
    printf("%s %s\n\n", "Message to be sent:", message);

    int i, j, k, location_receiver, location_sender, frame_size, location_by_using;
    char aa[msg_size];
    frame_size = strlen(message) / msg_size;

    //initializing frame size
    if (strlen(message) % msg_size > 0)
        frame_size++;

    if (strlen(message) < msg_size)
        frame_size = 1;

    //gets location of sender client and receiver client
    for (j = 0; j < client_size; j++){
        if (strcmp(client[j].client_id , sender_id) == 0)
            location_sender = j;
        else if (strcmp(client[j].client_id , receiver_id) == 0){
            location_receiver = j;      
        }      
    }

    //puts all data into frame
    for (i = 0; i < frame_size ; i++){ //loop for frames
        for (k = 0; k < 4; k++){ //loop for layers 
            if (k == 3){
                Frame fr;
                fr.flag = "Physical";
                strcpy(fr.phy.sender_mac, client[location_sender].client_mac);

                if (routeControl(location_receiver, location_sender) == location_receiver){
                    strcpy(fr.phy.receiver_mac, client[location_receiver].client_mac);
                }

                else{
                    strcpy(fr.phy.receiver_mac, client[routeControl(location_receiver, location_sender)].client_mac);
                }
                
                push(fr);
            }
            else if (k == 1){
                Frame fr;
                fr.flag = "Transport";
                strcpy(fr.tport.receiver_port , incoming_port);
                strcpy(fr.tport.sender_port ,  outgoing_port);
                push(fr);
            }
            else if (k == 2){
                Frame fr;
                fr.flag = "Network";
                strcpy(fr.netw.sender_ip, client[location_sender].client_ip);
                strcpy(fr.netw.receiver_ip, client[location_receiver].client_ip);
                push(fr);
            }
            else{
                Frame fr;
                fr.flag = "Application";
                strcpy(fr.app.sender_id, sender_id);
                strcpy(fr.app.receiver_id, receiver_id);
                memcpy(aa, message + msg_size * i, msg_size);
                strcpy(fr.app.message, aa);
                push(fr);
            }
        }

        //allocate memory based on frame size
        if (queue[location_sender].rear_outgoing == -1){
            queue[location_sender].outgoing_queue = malloc(frame_size * sizeof(Frame*));
            
            for (j = 0; j < frame_size ; j++){
                queue[location_sender].outgoing_queue[j] = malloc(4 * sizeof(Frame));
            }
        }

        //inserting frame into sender's outgoing queue
        insert_outcome(location_sender, frames);
        printf("Frame #%d\n", i+1);

        //pop frame
        for (j = 0; j < 4; j++){
            pop();
        }
    }
}

/*
*commandShowFrameInfo prints the data in frame.
*function gets that from queue, depending on which frame user wants.
*and checks if there is exist a such frame.
*/

void commandShowFrameInfo(char *client_id, char* whichqueue, int framenumber){
    int location, i;
    for (i = 0; i < client_size; i++){
        if (strcmp(client[i].client_id , client_id) == 0)
            location = i;
    }

    if (strcmp(whichqueue, "out") == 0){ //checks which queue it is
        printf("---------------------------------\n");
        printf("Command: SHOW_FRAME_INFO %s %s %d\n", client_id, whichqueue, framenumber);
        printf("---------------------------------\n");

        if (queue[location].rear_outgoing < framenumber - 1 || framenumber <= 0){
            printf("No such frame.");
        }

        else{
            printf("Current Frame #%d on the outgoing queue of client %s\n", framenumber, client_id);
            printf("Carried Message: \"%s\"\n", queue[location].outgoing_queue[framenumber-1][0].app.message);
            printf("Layer 0 info: Sender ID: %s, Receiver ID: %s\n", queue[location].outgoing_queue[framenumber-1][0].app.sender_id, queue[location].outgoing_queue[framenumber-1][0].app.receiver_id);
            printf("Layer 1 info: Sender port number: %s, Receiver port number: %s\n", queue[location].outgoing_queue[framenumber-1][1].tport.sender_port, queue[location].outgoing_queue[framenumber-1][1].tport.receiver_port);
            printf("Layer 2 info: Sender IP address: %s, Receiver IP address: %s\n", queue[location].outgoing_queue[framenumber-1][2].netw.sender_ip, queue[location].outgoing_queue[framenumber-1][2].netw.receiver_ip);
            printf("Layer 3 info: Sender MAC address: %s, Receiver MAC address: %s\n", queue[location].outgoing_queue[framenumber-1][3].phy.sender_mac, queue[location].outgoing_queue[framenumber-1][3].phy.receiver_mac);
            printf("Number of hops so far: %d", hop);
        }
    }

    else if (strcmp(whichqueue, "in") == 0){ //checks which queue it is
        printf("---------------------------------\n");
        printf("Command: SHOW_FRAME_INFO %s %s %d\n", client_id, whichqueue, framenumber);
        printf("---------------------------------\n");

        if (queue[location].rear_incoming < framenumber - 1 || framenumber <= 0){
            printf("No such frame.");
        }

        else{
            printf("Current Frame #%d on the incoming queue of client %s\n", framenumber, client_id);
            printf("Carried Message: \"%s\"\n", queue[location].incoming_queue[framenumber-1][0].app.message);
            printf("Layer 0 info: Sender ID: %s, Receiver ID: %s\n", queue[location].incoming_queue[framenumber-1][0].app.sender_id, queue[location].incoming_queue[framenumber-1][0].app.receiver_id);
            printf("Layer 1 info: Sender port number: %s, Receiver port number: %s\n", queue[location].incoming_queue[framenumber-1][1].tport.sender_port, queue[location].incoming_queue[framenumber-1][1].tport.receiver_port);
            printf("Layer 2 info: Sender IP address: %s, Receiver IP address: %s\n", queue[location].incoming_queue[framenumber-1][2].netw.sender_ip, queue[location].incoming_queue[framenumber-1][2].netw.receiver_ip);
            printf("Layer 3 info: Sender MAC address: %s, Receiver MAC address: %s\n", queue[location].incoming_queue[framenumber-1][3].phy.sender_mac, queue[location].incoming_queue[framenumber-1][3].phy.receiver_mac);
            printf("Number of hops so far: %d", hop);
        }
    }
}

/*
*commandShowQINFO prints the number of frames in that queue.
*/

void commandShowQInfo(char *client_id, char* whichqueue){
    if (strcmp(whichqueue, "out\n") == 0)
        whichqueue = "out";
    
    else if (strcmp(whichqueue, "in\n") == 0)
        whichqueue = "in";
    
    printf("--------------------------\n");
    printf("Command: SHOW_Q_INFO %s %s\n", client_id, whichqueue);
    printf("--------------------------\n");
    
    int i, location;

    for (i = 0 ; i < client_size ; i++){ //initialize the location of client
        if (strcmp(client[i].client_id, client_id) == 0){
            location = i;
        }
    }

    if (strcmp(whichqueue, "out") == 0){
        printf("Client %s Outgoing Queue Status\n", client_id);
        printf("Current total number of frames: %d", queue[location].outsize);
    }

    else if (strcmp(whichqueue, "in") == 0){
        printf("Client %s Incoming Queue Status\n", client_id);
        printf("Current total number of frames: %d", queue[location].insize);
    }
}

/*
*createLog puts all data in log array.
*and gets these datas from queues.
*/

void createLog(int location, char *activity_type, char *success_status){
    time_t current_time;
    struct tm *tm;

    int i,j;

    if (strcmp(activity_type, "Message Forwarded") == 0){ //if message is forwarded, then it will be 2 log
        logs[location].log_str = malloc(2 * sizeof(Log));
        logs[location].entry_size = 2;

        time(&current_time);
        tm = localtime(&current_time);

        for (i = 0; i < logs[location].entry_size; i++){
            strftime(logs[location].log_str[i].timme, 50, "%H:%M:%S", tm);
            strftime(logs[location].log_str[i].date, 50, "%d-%m-%Y", tm);

            strcpy(logs[location].log_str[i].sender_id, queue[location].outgoing_queue[0][0].app.sender_id);
            strcpy(logs[location].log_str[i].receiver_id, queue[location].outgoing_queue[0][0].app.receiver_id);

            strcpy(logs[location].log_str[i].message , "");

            for (j = 0; j < queue[location].outsize; j++){
                strcat(logs[location].log_str[i].message, queue[location].outgoing_queue[j][0].app.message);
            }

            logs[location].log_str[i].total_frames = queue[location].outsize;
            logs[location].log_str[i].total_hops = hop;

            if (i == 0){
                strcpy(logs[location].log_str[i].activity_type, "Message Received"); //firstly, client gets message (received), after that it forwards.
            }
            else{
                strcpy(logs[location].log_str[i].activity_type, activity_type);
            }
            
            strcpy(logs[location].log_str[i].success_status, success_status);
        }

    }
    else{ //if message is received or sent, then it will be 1 log
        logs[location].log_str = malloc(1 * sizeof(Log));
        logs[location].entry_size = 1;

        time(&current_time);
        tm = localtime(&current_time);

        strftime(logs[location].log_str[0].timme, 100, "%H:%M:%S", tm);
        strftime(logs[location].log_str[0].date, 100, "%d-%m-%Y", tm);

        strcpy(logs[location].log_str[0].activity_type, activity_type);
        strcpy(logs[location].log_str[0].success_status, success_status);
        logs[location].log_str[0].total_hops = hop;

        if (strcmp(activity_type, "Message Received") == 0){ //if message is received, function must get datas from incoming queue
            strcpy(logs[location].log_str[0].sender_id, queue[location].incoming_queue[0][0].app.sender_id);
            strcpy(logs[location].log_str[0].receiver_id, queue[location].incoming_queue[0][0].app.receiver_id);
            strcpy(logs[location].log_str[0].message , "");

            for (i = 0; i < queue[location].insize; i++){
                strcat(logs[location].log_str[0].message, queue[location].incoming_queue[i][0].app.message);
            }

            logs[location].log_str[0].total_frames = queue[location].insize;
        }
        
        else{ //if message is sent, function must get datas from outgoing queue
            strcpy(logs[location].log_str[0].sender_id, queue[location].outgoing_queue[0][0].app.sender_id);
            strcpy(logs[location].log_str[0].receiver_id, queue[location].outgoing_queue[0][0].app.receiver_id);
            strcpy(logs[location].log_str[0].message , "");

            for (i = 0; i < queue[location].outsize; i++){
                strcat(logs[location].log_str[0].message, queue[location].outgoing_queue[i][0].app.message);
            }

            logs[location].log_str[0].total_frames = queue[location].outsize;
        }
    }
}

/*
*messageControl function controls that receiver mac and receiver id is same or not.
*if they are same, that means message reached the client.
*if they are not same, must initialize a client that must use to reach receiver id.
*calls route control function and gets the location of client that be used.
*now, using client will be sender client. and must get the new client to be used.
*puts all frames into outgoing queue from incoming queue.
*now, queues are ready but receiver mac and receiver id not paired with.
*so, messageControl function must be called again until receiver mac and receiver id is paired.
*/

void messageControl(int location_sender, int location_receiver, int msg_size){
    int i, location_by_using, size;

    if (strcmp(queue[location_sender].outgoing_queue[0][3].phy.receiver_mac, client[location_receiver].client_mac) == 0){
        hop++;

        size = queue[location_sender].outsize; //size of frames

        queue[location_receiver].incoming_queue = malloc(size * sizeof(Frame*));

        for (i = 0; i < size; i++){
            queue[location_receiver].incoming_queue[i] = malloc(4 * sizeof(Frame));
            insert_income(location_receiver, peek_outcome(location_sender));
            remove_outcome(location_sender);
        }
        
        printf("A message received by client %s from client %s after a total %d hops.\n", client[location_receiver].client_id, queue[location_receiver].incoming_queue[0][0].app.sender_id, hop);

        createLog(location_receiver, "Message Received", "Yes");
        printf("Message: ");

        for (i = 0; i < size; i++){
            printf("%s", queue[location_receiver].incoming_queue[i][0].app.message);
        }

        printf("\n");
        free(queue[location_sender].outgoing_queue); //free the outgoing queue of sender client
    }

    else{ //if receiver mac does not paired with
        location_by_using = routeControl(location_receiver, location_sender); //location of client that be used
        printf("A message received by client %s, but intended for client %s. Forwarding...\n", client[location_by_using].client_id, client[location_receiver].client_id);

        if (location_by_using != -1){ 
            queue[location_by_using].incoming_queue = malloc(queue[location_sender].outsize * sizeof(Frame *));
            hop++;

            size = queue[location_sender].outsize;

            for (i = 0; i < size; i++){ //puts all frames into client that be used's incoming queue
                queue[location_by_using].incoming_queue[i] = malloc(4 * sizeof(Frame));
                insert_income(location_by_using, peek_outcome(location_sender));
                remove_outcome(location_sender); //removes from sender outgoing queue
            }

            free(queue[location_sender].outgoing_queue);

            queue[location_by_using].outgoing_queue = malloc(size * sizeof(Frame*));

            location_sender = location_by_using;
            location_by_using = routeControl(location_receiver, location_sender); //location by using must change

            if (location_by_using != -1){

            	for (i = 0; i < size; i++){ //puts frames to client to be used's outgoing queue from same client's incoming queue
	                printf("\tFrame #%d MAC address change: New sender MAC %s, new receiver MAC %s\n", i+1, client[location_sender].client_mac, client[location_by_using].client_mac);
	                queue[location_sender].outgoing_queue[i] = malloc(4 * sizeof(Frame));
	                insert_outcome(location_sender, peek_income(location_sender));
	                strcpy(queue[location_sender].outgoing_queue[i][3].phy.receiver_mac , client[location_by_using].client_mac);
	                remove_income(location_sender);
            	}

            	createLog(location_sender, "Message Forwarded", "Yes"); //creates log depending on forwarded message
            	free(queue[location_sender].incoming_queue);
            	messageControl(location_sender, location_receiver, msg_size); //must call again due to receiver mac and receiver id not paired
            }

            else{
            	hop++;
            	printf("Error: Unreachable destination. Packets are dropped after %d hops!", hop);
            	createLog(location_by_using, "Message Forwarded", "No");
            	free(queue[location_sender].outgoing_queue);
            }
            
        }

        else{
            hop++;
            printf("Error: Unreachable destination. Packets are dropped after %d hops!", hop);
            createLog(location_by_using, "Message Forwarded", "No");
            free(queue[location_sender].outgoing_queue);
        }
    }

}

/*
*sendCommand gets the location of clients that sender and receiver
*creates log based on message sent.
*and calls the messageControl function.
*/

void sendCommand(char *client_id, int msg_size){
    printf("----------------\n");
    printf("Command: SEND %c\n", client_id[0]);
    printf("----------------\n");
    
    int i, location_sender, location_receiver;

    for (i = 0; i < client_size; i++){
        if ((client[i].client_id)[0] == client_id[0]){
            location_sender = i;
        }
    }

    for (i=0; i < client_size; i++){
        if (strcmp(queue[location_sender].outgoing_queue[0][0].app.receiver_id, client[i].client_id) == 0)
            location_receiver = i;
    }

    createLog(location_sender, "Message Sent", "Yes");
    messageControl(location_sender, location_receiver, msg_size);
}

/*
*printLog function prints all the data of log of chosen client id.
*/

void printLog(char *client_id){
    printf("---------------------\n");
    printf("Command: PRINT_LOG %c\n", client_id[0]);
    printf("---------------------\n");
    printf("Client %c Logs:\n", client_id[0]);
    
    int i, location;

    for (i = 0; i < client_size; i++){
        if ((client[i].client_id)[0] == client_id[0]){
            location = i;
        }
    }

    for (i = 0; i < logs[location].entry_size; i++){
        printf("---------------------\n");
        printf("Log Entry #%d:\n", i+1);
        printf("Timestamp: %s %s\n", logs[location].log_str[i].date ,logs[location].log_str[i].timme);
        printf("Message: %s\n", logs[location].log_str[i].message);
        printf("Number of frames: %d\n", logs[location].log_str[i].total_frames);
        printf("Number of hops: %d\n", logs[location].log_str[i].total_hops);
        printf("Sender ID: %s\n", logs[location].log_str[i].sender_id);
        printf("Receiver ID: %s\n", logs[location].log_str[i].receiver_id);
        printf("Activity: %s\n", logs[location].log_str[i].activity_type);
        printf("Success: %s\n", logs[location].log_str[i].success_status);
    }
}

/*
*readCommands reads the commands file, and calls the function,
*based on which one is called.
*/

void readCommands(FILE *commandFile, int msg_size, char* incoming_port, char* outgoing_port){
    int size, i;
    char line[1000];
    char *token;
    char *cid, *whichqueue;

    fgets(line, sizeof line, commandFile);
    size = atoi(line);

    for (i = 0; i < size; i++){
        fgets(line, sizeof line, commandFile);
        token = strtok(line, " ");

        if (strcmp(token, "MESSAGE") == 0){
            token = strtok(NULL, " ");
            char *sid = token;
            token = strtok(NULL, " ");
            char *rid = token;
            token = strtok(NULL, "#");
            char *msg = token;
            
            commandMessage(sid, rid, msg, msg_size, incoming_port, outgoing_port);
        }

        else if (strcmp(token, "SHOW_FRAME_INFO") == 0){
            token = strtok(NULL, " ");
            cid = token;
            token = strtok(NULL, " ");
            whichqueue = token;
            token = strtok(NULL, " ");
            int framenumber = atoi(token);

            commandShowFrameInfo(cid, whichqueue, framenumber);

            if (i != size-1)
                printf("\n");
        }

        else if (strcmp(token, "SHOW_Q_INFO") == 0){
            token = strtok(NULL, " ");
            cid = token;
            token = strtok(NULL, " ");
            whichqueue = token;

            commandShowQInfo(cid, whichqueue);

            if (i != size-1)
                printf("\n");
        }

        else if (strcmp(token, "SEND") == 0){
            token = strtok(NULL, " ");
            cid = token;

            sendCommand(cid, msg_size);
        }

        else if (strcmp(token, "PRINT_LOG") == 0){
            token = strtok(NULL, " ");
            cid = token;
            
            printLog(cid);
        }

        else{
            printf("---------------------\n");
            printf("Command: %s ",token);
            token = strtok(NULL, " ");

            while (token != NULL){
                printf("%s ", token);
                token = strtok(NULL, " ");
            }
            printf("\n---------------------\n");
            printf("Invalid command.");

            if (i != size-1)
                printf("\n");
        }
    }
}

int main(int argc, char *argv[]){
    FILE *clientFile,
         *routingFile,
         *commandFile;
    
    clientFile = fopen(argv[1], "r");
    routingFile = fopen(argv[2], "r");
    commandFile = fopen(argv[3], "r");

    int msg_size = atoi(argv[4]);
    char outgoing_port[5];
    char incoming_port[5];
    
    strcpy(outgoing_port, argv[5]);
    strcpy(incoming_port, argv[6]);
    int i, location;
    
    readClientsRouting(clientFile, routingFile);
    readCommands(commandFile, msg_size, incoming_port, outgoing_port);

    for (i = 0; i < client_size; i++){
        if (queue[i].insize > 0)
            location = i;
    }

    //free the receiver's incoming queue after all commands are finished
    for (i = 0; i < queue[location].insize ; i++){
        free(queue[location].incoming_queue[i]);
    }

    free(queue[location].incoming_queue);

    fclose(clientFile);
    fclose(routingFile);
    fclose(commandFile); 

    //free all arrays
    for (i = 0; i < client_size; i++){
        free(route[i]);
        free(logs[i].log_str);
    }

    free(client);
    free(queue);
        
    free(logs);
    free(route);

}