#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// #define MYDEBUG

/* a rtpkt is the packet sent from one router to
   another*/
struct rtpkt {
    int sourceid;       /* id of sending router sending this pkt */
    int destid;         /* id of router to which pkt being sent 
                         (must be an directly connected neighbor) */
    int *mincost;    /* min cost to all the node  */
};


struct distance_table{
    int **costs;     // the distance table of curr_node, costs[i][j] is the cost from node i to node j
};


struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity (node) where event occurs */
    struct rtpkt *rtpktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL;   /* the event list */
struct distance_table *dts;
int **link_costs; /*This is a 2D matrix stroing the content defined in topo file*/
int num_nodes;

/* possible events: */
/*Note in this lab, we only have one event, namely FROM_LAYER2.It refer to that the packet will pop out from layer3, you can add more event to emulate other activity for other layers. Like FROM_LAYER3*/
#define  FROM_LAYER2     1
#define SLOT_END 2

float clocktime = 0.000;

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
void insertevent(struct event *p)
{
    struct event *q,*qold;
    q = evlist;     /* q points to header of list in which p struct inserted */
    
    if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
    } else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
            qold=q; 
        if (q==NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q==evlist) { /* front of list */
            p->next=evlist;
            p->prev=NULL;
            p->next->prev=p;
            evlist = p;
        } else {     /* middle of list */
            // don't put a event in front of a slot end 
            if(q->evtype == SLOT_END){

            }
            p->next=q;
            p->prev=q->prev;
            q->prev->next=p;
            q->prev=p;
        }
    }
}


/************************** send update to neighbor (packet.destid)***************/
void send2neighbor(struct rtpkt *packet){
    struct event *evptr, *q;
    float jimsrand(),lastime, lastendtime;
    int i;
    
    /* be nice: check if source and destination id's are reasonable */
    if (packet->sourceid<0 || packet->sourceid >num_nodes) {
        printf("WARNING: illegal source id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->destid<0 || packet->destid > num_nodes) {
        printf("WARNING: illegal dest id in your packet, ignoring packet!\n");
        return;
    }
    if (packet->sourceid == packet->destid)  {
        printf("WARNING: source and destination id's the same, ignoring packet!\n");
        return;
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype =  FROM_LAYER2;   /* packet will pop out from layer3 */
    evptr->eventity = packet->destid; /* event occurs at other entity */
    evptr->rtpktptr = packet;       /* save ptr to my copy of packet */

    /* finally, compute the arrival time of packet at the other end.
      medium can not reorder, so make sure packet arrives between 1 and 10
      time units after the latest arrival time of packets
      currently in the medium on their way to the destination */
    lastime = clocktime;
    lastendtime = 0;
    for(q=evlist; q!= NULL; q = q->next){
        if(q->evtype == SLOT_END)
            lastendtime = q->evtime;
    }
    for (q=evlist; q!=NULL ; q = q->next) 
        if ( (q->evtype==FROM_LAYER2  && q->eventity==evptr->eventity) ) 
            lastime = q->evtime;

    float randomtime = lastime + 2.*jimsrand();
    if(randomtime < lastendtime){
        evptr->evtime = lastendtime + 0.1*jimsrand();
    } else {
        evptr->evtime = randomtime;
    }
    insertevent(evptr);
}


int min_(int x, int y){
    if(x < y && x >= 0)
        return x;
    else
        return y;
}

bool check_update(int x, int y){
    if(x <= y && x >= 0)
        return false;
    else
        return true;
}

void rtinit(struct distance_table *dt, int node, int *link_costs, int num_nodes){
    /* Todo: Please write the code here*/
    // allocate space
    dt->costs = (int **) malloc(num_nodes * sizeof(int *));
    for (int i = 0; i < num_nodes; i++){
        dt->costs[i] = (int *)malloc(num_nodes * sizeof(int));
    }
    // Initialize the values
    for(int i = 0; i < num_nodes; i++){
        for(int j = 0; j < num_nodes; j++)
            dt->costs[i][j] = -1;
    }
    for(int i = 0; i < num_nodes; i++){
        dt->costs[node][i] = link_costs[i];
    }
#ifdef MYDEBUG
    // print_dt(dt, node);
#endif
    //TODO: send it send2neighbor
    for(int i = 0; i < num_nodes; i++){
        if(link_costs[i] > 0){
            struct rtpkt* send_pkt = (struct rtpkt*)malloc(sizeof(struct rtpkt));
            send_pkt->sourceid = node;
            // send_pkt->destid = -1; //flag
            send_pkt->destid = i;
            send_pkt->mincost = dt->costs[node];
            // printf("%d->%d\n", send_pkt->sourceid, send_pkt->destid);
            send2neighbor(send_pkt);
        }
    }
}

void rtupdate(struct distance_table *dt, struct rtpkt recv_pkt){
    /* Todo: Please write the code here*/
    int src = recv_pkt.sourceid;
    int cur = recv_pkt.destid;
    // printf("I'm node %d:\n", cur);
    bool update_flag = false;
    for(int i = 0; i < num_nodes; i++){
        // printf("Update cost to node %d: lc(%d, %d)=%d, get new c(%d, %d)=%d\n", i, cur, src,link_costs[src][cur],
        //                                 src, i, recv_pkt.mincost[i]);
        if(recv_pkt.mincost[i] > 0)
            if(check_update(dt->costs[cur][i],
                            link_costs[src][cur] + recv_pkt.mincost[i])){
                update_flag = true;
                // printf("update(%d, %d): %d->%d\n", cur, i, dt->costs[cur][i], link_costs[src][cur] + recv_pkt.mincost[i]);
                dt->costs[cur][i] = link_costs[src][cur] + recv_pkt.mincost[i];
            }
    }
    if(update_flag){
        // printf("[node %d update dv]\n", cur);
        for(int i = 0; i < num_nodes; i++){
            if(link_costs[cur][i] > 0){
                struct rtpkt* send_pkt = (struct rtpkt*)malloc(sizeof(struct rtpkt));
                send_pkt->sourceid = cur;
                // send_pkt->destid = -1; //flag
                send_pkt->destid = i;
                send_pkt->mincost = dt->costs[cur];
                // printf("%d->%d\n", send_pkt->sourceid, send_pkt->destid);
                send2neighbor(send_pkt);
                // exit(0);
            }
        }
    }
}


/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
float jimsrand() {
    double mmm = 2147483647;   
    float x;                   
    x = rand()/mmm;            
    return(x);
}  


/************** Some printers *******************/
void print_dt(struct distance_table *dt, int node){
    printf("===node %d===\n", node);
    for(int i = 0; i < num_nodes; i++){
        for(int j = 0; j < num_nodes; j++)
            printf("%d ", dt->costs[i][j]);
        printf("\n");
    }
}


void printevent(){
    struct event *q, *qold;
    q = evlist;
    while(q!=NULL){
        if(q->evtype == FROM_LAYER2)
            printf("time %.4f: %d -> %d \n", q->evtime, q->rtpktptr->sourceid,
                                    q->rtpktptr->destid);
        else    
            printf("---SLOT finished---\n");
        q = q->next;
    }
}

void print_all_dv(int slot){
    printf("k=%d:\n", slot);
    for(int i = 0; i < num_nodes; i++){
        printf("node-%d:", i);
        for(int j = 0; j < num_nodes; j++){
            printf(" %d", dts[i].costs[i][j]);
        }
        printf("\n");
    }
}

void insert_slot_barrier(){
    struct event *eptr, *q;
    eptr = (struct event *)malloc(sizeof(struct event));
    eptr->evtype = SLOT_END;
    float lastime = clocktime;
    for (q=evlist; q!=NULL; q = q->next) 
        lastime = q->evtime;
    eptr->evtime = lastime + 0.00001;
    insertevent(eptr);
}
 

void get_num_nodes(char* path){
    int read_len = 0;
    size_t buf_len = 0;
    char* buf = NULL;
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        perror("open file");
    while((read_len = getline(&buf, &buf_len, fp)) != -1){
        num_nodes++;
    }
    if(buf)
        free(buf);
}

void get_link_cost(char* path, int** link_costs, int n){
    /* Initialize link_cost with the topology file*/
    int read_len = 0;
    size_t buf_len = 0;
    char* buf = NULL;
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        perror("open file");

    for(int i = 0; i < n; i++){
        // ith line
        if((read_len = getline(&buf, &buf_len, fp)) == -1)
            perror("getline");
        int cur_head = 0;
        for(int j = 0; j < n; j++){
            // jth number
            int num_len = 0;
            while(buf[cur_head + num_len] != ' ' &&
                    buf[cur_head + num_len] != '\n' &&
                    buf[cur_head + num_len] != '\r' &&
                    buf[cur_head + num_len] != '\0'){
                num_len++;
            } 
            if(buf[cur_head + num_len] != ' ' && j < n - 1){
                // this line has less than n numbers
                // printf("head: %d, len: %d\n", cur_head, num_len);
                perror("invalid topology 1");
            }
            if(num_len == 0){
                // printf("head: %d, len: %d\n", cur_head, num_len);
                perror("invalid topology 2");
            }

            char temp_num[10];
            strncpy(temp_num, buf + cur_head, num_len);
            temp_num[num_len] = '\0';
            link_costs[i][j] = atoi(temp_num);
            cur_head += num_len + 1;
        }
    }

}


void main(int argc, char *argv[])
{
    struct event *eventptr;

    int k_max = 0;
    int cur_slot = 0;
    k_max = atoi(argv[1]);
    get_num_nodes(argv[2]);
    
    dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));
    link_costs = (int **) malloc(num_nodes * sizeof(int *));
    for (int i = 0; i < num_nodes; i++){
        link_costs[i] = (int *)malloc(num_nodes * sizeof(int));
    }
    get_link_cost(argv[2], link_costs, num_nodes);

    for (int i = 0; i < num_nodes; i++){
        rtinit(&dts[i], i, link_costs[i], num_nodes);
    }
    insert_slot_barrier();
    print_all_dv(0);

    
    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
            evlist->prev=NULL;
        clocktime = eventptr->evtime;    /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER2 ) {
            /* Todo: You need to modify the rtupdate method and add more codes here for Part B and Part C, since the link costs in these parts are dynamic.*/
            rtupdate(&dts[eventptr->eventity], *(eventptr->rtpktptr));
        }
        else if(eventptr->evtype == SLOT_END){
            /* End of a slot*/
            cur_slot += 1;
            if(cur_slot <= 4 || cur_slot % 10 == 0)
                print_all_dv(cur_slot);
            // printevent();
            insert_slot_barrier();
            if(cur_slot >= k_max){
                goto finished;
            }
        } else {
            printf("Panic: unknown event type\n"); 
            exit(0);
        }
        if (eventptr->evtype == FROM_LAYER2 ) 
          free(eventptr->rtpktptr);        /* free memory for packet, if any */
        free(eventptr);                    /* free memory for event struct   */
    }
   

terminate:
    printf("\nSimulator terminated at t=%f, no packets in medium\n", clocktime);
    return;

finished:
#ifdef MYDEBUG
    printf("\nSimulator terminated at t=%f\n", clocktime);
#endif
    return;
}


