#include<stdarg.h>
#include<signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>
#include <unistd.h>

#ifdef NETBEANS
#include "ServerProgram/wiringPi.h"
#else
#include <wiringPi.h>
#endif

#include<time.h>

#include <string.h>
#include <sys/time.h>
#define DEBUG_MODE
#define MAXCLIENTS  20

#define INPUTPIN1   0
#define INPUTPIN2   1
#define INPUTPIN3   2
#define INPUTPIN4   3

//#define va_end(ap)              (void) 0
//#define va_start(ap, A)         (void) ((ap) = (((char *) &(A)) + (_bnd (A,_AUPBND))))
#define SERVER_STATUS_CODE_INIT                 -1
#define SERVER_STATUS_CODE_INITIATING            0
#define SERVER_STATUS_CODE_RUNNING_SIM_GR        1
#define SERVER_STATUS_CODE_RUNNING_SIM_SG        2
#define SERVER_STATUS_CODE_RUNNING_RUN_GR        3
#define SERVER_STATUS_CODE_RUNNING_RUN_SG        4
#define SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET  5
#define SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET  6
#define SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET  7
#define SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET  8
#define SERVER_STATUS_CODE_RESET_MODE            9

#define PREFERRED_WORKING_DIRECTORY              home/pi/imagica/programs/serverPrograms

#define GOLD_RUSH_SIMULATION_FILE


#define SIMULATION_DECIMATION_TIME          4000
#define INTERRUPT_MODE

struct ClientStatus {
    int IsConnected;
    pthread_t ThreadId;
    int ClientSocketFileDiscriptor, ClientLength;
    struct sockaddr_in ClientAddress;
    int ClientIP[4],ResponseAvailable,DenyInput;
    char Response[100];
};

int Server_Status = SERVER_STATUS_CODE_INIT;

int GoldRushSimValues[70000], GoldRushNOV;
int SalimGharSimValues[150000], SalimGharNOV;

int InputPinLog[4][150000], InputPinLogPointer[4], InputPinLogPointerPrevious[4];

int TranssferCount;

float distance, SalimGharIncrement = 0, GoldRushIncrement = 0;


struct sigaction InputSigAction, ServerSigAction;
int ServerSocketFileDiscriptor, PortNumber = 1234, ClientCount = 0, MaximumFileDiscriptorID, ReUseSocket = 1, ConnectedClients = 0;
int SimulationStartTime, PreviousTransmitTime, PreviousTime, PreviousLocation = 0, location;
int SalimgharTrackCount = 8750, GoldRushTrackCount = 4000;
struct sockaddr_in ServerAddress;
struct timeval Timer_Variable;
struct itimerval ServerTimer, InputTimer;
struct ClientStatus Clients[MAXCLIENTS + 1], FileCheckStruct;
fd_set ReadFileDiscriptors, ExceptFileDiscriptors;
char InputDataBuffer[256], OutputDataBuffer[256], Address[20], ConnectedAddresses[500];

int stopeverything = 0, servercount = 0, inputcount = 0;

int InputPipe[2], ServerPipe[2];

void InputCounter1(void) {
    InputPinLogPointer[0]++;
    InputPinLog[0][InputPinLogPointer[0]] = micros();
}

void InputCounter2(void) {
    InputPinLogPointer[1]++;
    InputPinLog[1][InputPinLogPointer[1]] = micros();
}

void InputCounter3(void) {
    InputPinLogPointer[2]++;
    InputPinLog[2][InputPinLogPointer[2]] = micros();
}

void InputCounter4(void) {
    InputPinLogPointer[3]++;
    InputPinLog[3][InputPinLogPointer[3]] = micros();
}

void printspecial(int priority, char *format, ...) {
    va_list arg;
    va_start(arg, *format);
    if (priority > 0)
        vfprintf(stdout, format, arg);
    va_end(arg);
}

void sigquit() {
    printspecial(0, "My DADDY has Killed me!!!\n");
    stopeverything = 1;
}

void ResetInput() {
    write(InputPipe[1], &Server_Status, sizeof (Server_Status));
    InputPinLogPointer[0] = 0;
    InputPinLogPointer[1] = 0;
    InputPinLogPointer[2] = 0;
    InputPinLogPointer[3] = 0;
    location=0;
    while (location != -10) {

        Timer_Variable.tv_sec = 0;
        Timer_Variable.tv_usec = 10;
        FD_ZERO(&ReadFileDiscriptors);
        FD_SET(ServerPipe[0], &ReadFileDiscriptors);
        MaximumFileDiscriptorID = ServerPipe[0];
        if (select(MaximumFileDiscriptorID + 1, &ReadFileDiscriptors, NULL, NULL, &Timer_Variable) == 0) {
            printspecial(0, "%d\n", send);
            continue;
        }
        read(ServerPipe[0], &location, sizeof (location));
    }
    location=0;
}

void LoadTrackCount() {
    FILE *Reader;
    if (stat("/home/pi/imagica/programs/serverProgram/SalimGharTrackCount.txt", &FileCheckStruct) == 0) {
        Reader = fopen("/home/pi/imagica/programs/serverProgram/SalimGharTrackCount.txt", "r");
        fscanf(Reader, "%d\n", &SalimgharTrackCount);
        fclose(Reader);
        if (SalimgharTrackCount < 1) {
            SalimgharTrackCount = 8750;
        }
    }

    SalimGharIncrement = 267000.0 / (float) SalimgharTrackCount;

    if (stat("/home/pi/imagica/programs/serverProgram/GoldRushTrackCount.txt", &FileCheckStruct) == 0) {
        Reader = fopen("/home/pi/imagica/programs/serverProgram/GoldRushTrackCount.txt", "r");
        fscanf(Reader, "%d\n", &GoldRushTrackCount);
        fclose(Reader);
        if (GoldRushTrackCount < 1) {
            GoldRushTrackCount = 4000;
        }
    }
    GoldRushIncrement = 100000.0 / (float) GoldRushTrackCount;
}

void StoreTrackCount() {/*
    FILE *Writer;
    Writer = fopen("/home/pi/imagica/programs/serverProgram/SalimGharTrackCount.txt", "w");
    fprintf(Writer, "%d\n", SalimgharTrackCount);
    fclose(Writer);
    if (SalimgharTrackCount < 1) {
        SalimgharTrackCount = 8750;
    }

    Writer = fopen("/home/pi/imagica/programs/serverProgram/GoldRushTrackCount.txt", "w");
    fprintf(Writer, "%d\n", GoldRushTrackCount);
    fclose(Writer);
    if (GoldRushTrackCount < 1) {
        GoldRushTrackCount = 4000;
    }*/
}

int main(int argc, char *argv[]) {
    FILE *Reader, *Writer;
    char InputString[600];
    int OldManVideoTime = 0;
    printspecial(0, "reading goldrush values\n");
    long iterator1;
    Server_Status = SERVER_STATUS_CODE_INITIATING;
    Reader = fopen("/home/pi/imagica/programs/serverProgram/goldrushvalues.txt", "r");
    fscanf(Reader, "%d\n", &GoldRushNOV);
    for (iterator1 = 0; iterator1 < GoldRushNOV; iterator1++) {
        fscanf(Reader, "%d\n", &(GoldRushSimValues[iterator1]));
    }
    fclose(Reader);

    printspecial(0, "reading salimghar values\n");
    Reader = fopen("/home/pi/imagica/programs/serverProgram/salimgharvalues.txt", "r");
    fscanf(Reader, "%d\n", &SalimGharNOV);
    for (iterator1 = 0; iterator1 < SalimGharNOV; iterator1++) {
        fscanf(Reader, "%d\n", &(SalimGharSimValues[iterator1]));
    }
    fclose(Reader);

    LoadTrackCount();

    printspecial(0, "Clearing clients\n");
    for (ClientCount = 0; ClientCount < MAXCLIENTS + 1; ClientCount++) {
        Clients[ClientCount].IsConnected = 0;
    }

    if (wiringPiSetup() == -1) {
        printspecial(0, "Wiring Pi did not load\n");
        return 1;
    }
    if (argc > 2) {
        sscanf(argv[2], "%d", &iterator1);
        if (iterator1 == 1) {
            Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
            distance = -2;
        } else {
            Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
        }
    } else {
        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
    }

    if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET) {
        printspecial(0, "Running GoldRush Simulation\n");
    } else {
        printspecial(0, "Running SalimGhar Simulation\n");
    }

    printspecial(0, "Starting Simulation\n");

    //printspecial(0,"Sigkill %d, SigStop %d SigAlrm %d, SigVTAlrm %d, SIGPROF %d, ITIMER_VIRTUAL %d, ITIMER_REAL %d\n", SIGKILL, SIGSTOP, SIGALRM, SIGVTALRM, SIGPROF, ITIMER_VIRTUAL, ITIMER_REAL); //9 19 14 26 27


    printspecial(0, "forking\n");

    int pid, data = 0;
    pipe(InputPipe);
    pipe(ServerPipe);
    pid = fork();
    if (pid == 0) {
        int CurrentTime, sucks = 0;
        char Command[50];
        close(InputPipe[1]);
        close(ServerPipe[0]);

        pinMode(INPUTPIN1, INPUT);
        pinMode(INPUTPIN2, INPUT);
        pinMode(INPUTPIN3, INPUT);
        pinMode(INPUTPIN4, INPUT);
        sprintf(Command,"gpio mode %d down",INPUTPIN1);
        system(Command);
        sprintf(Command,"gpio mode %d down",INPUTPIN2);
        system(Command);
        sprintf(Command,"gpio mode %d down",INPUTPIN3);
        system(Command);
        sprintf(Command,"gpio mode %d down",INPUTPIN4);
        system(Command);
        InputPinLogPointer[0] = 0;
        InputPinLogPointer[1] = 0;
        InputPinLogPointer[2] = 0;
        InputPinLogPointer[3] = 0;
        if (wiringPiISR(INPUTPIN1, INT_EDGE_BOTH, &InputCounter1) != 0) {
            printspecial(0, "Wiring Pi isr did not load on pin 1\n");
            return 1;
        }/*
        if (wiringPiISR(INPUTPIN2, INT_EDGE_BOTH, &InputCounter2) != 0) {
            printspecial(0, "Wiring Pi isr did not load on pin 2\n");
            return 1;
        }
        if (wiringPiISR(INPUTPIN3, INT_EDGE_BOTH, &InputCounter3) != 0) {
            printspecial(0, "Wiring Pi isr did not load on pin 3\n");
            return 1;
        }
        if (wiringPiISR(INPUTPIN4, INT_EDGE_BOTH, &InputCounter4) != 0) {
            printspecial(0, "Wiring Pi isr did not load on pin 4\n");
            return 1;
        }*/
        printspecial(0, "forked into child\n");
        read(InputPipe[0], &data, sizeof (data));
        SimulationStartTime = micros();
        PreviousTime = SimulationStartTime - SIMULATION_DECIMATION_TIME;
        PreviousTransmitTime = SimulationStartTime - 50000;
        while (1) {
            CurrentTime = micros();
            if ((Server_Status > 0)&&(Server_Status < 3)) {
                if ((CurrentTime - PreviousTime) > SIMULATION_DECIMATION_TIME) {
                    location = (CurrentTime - SimulationStartTime) / SIMULATION_DECIMATION_TIME; //&&(( CurrentTime - SimulationStartTime )%SIMULATION_DECIMATION_TIME)<1000){
                    if ((location - PreviousLocation) > 1) {
                        sucks += (location - PreviousLocation);
                        printspecial(1, "This sucks even more %d,timediff =%d\n", sucks, CurrentTime - PreviousTransmitTime);
                        PreviousTransmitTime = CurrentTime;
                    }
                    PreviousTime = (SIMULATION_DECIMATION_TIME * location) + SimulationStartTime;
                    PreviousLocation = location;
                    write(ServerPipe[1], &location, sizeof (location));
                }
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG) {
                location = InputPinLogPointer[0];
                if (location != PreviousLocation) {
                    printspecial(3, "location changed to %d\n", location);
                    write(ServerPipe[1], &location, sizeof (location));
                }
                PreviousLocation = location;
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR) {
                location = (InputPinLogPointer[0] +
                        InputPinLogPointer[1] +
                        InputPinLogPointer[2] +
                        InputPinLogPointer[3]);
                if (location != PreviousLocation) {
                    write(ServerPipe[1], &location, sizeof (location));
                    printspecial(3, "location changed to %d\n", location);
                }
                PreviousLocation = location;
            }
            Timer_Variable.tv_sec = 0;
            Timer_Variable.tv_usec = 10;
            FD_ZERO(&ReadFileDiscriptors);
            FD_SET(InputPipe[0], &ReadFileDiscriptors);
            if (select(InputPipe[0] + 1, &ReadFileDiscriptors, NULL, NULL, &Timer_Variable) != 0) {
                int prevserver = Server_Status;
                read(InputPipe[0], &Server_Status, sizeof (Server_Status));
                printspecial(3, "Got this is server status in input coder %d\n", Server_Status);
                if (Server_Status > 4) {
                    switch (prevserver) {
                        case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                            GoldRushTrackCount = location;
                            /* Attention remember top write code to store the logs*/
                            break;
                        case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                            SalimgharTrackCount = location;
                            /* Attention remember top write code to store the logs*/
                            break;
                        default:
                            break;

                    }
                    StoreTrackCount();
                }
                switch(Server_Status){
                        case SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET:
                            InputPinLogPointer[0]=0;
                            InputPinLogPointer[1]=0;
                            InputPinLogPointer[2]=0;
                            InputPinLogPointer[3]=0;
                            /* Attention remember top write code to store the logs*/
                            break;
                        case SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET:
                            InputPinLogPointer[0]=0;
                            /* Attention remember top write code to store the logs*/
                            break;
                        default:
                            break;

                    }
                location = -10;
                write(ServerPipe[1], &location, sizeof (location));
                location = 0;
                sleep(2);
                SimulationStartTime = micros();
                PreviousTime = SimulationStartTime;
                PreviousLocation = 0;
            }

            usleep(1000);
        }
        return;
    }
    close(InputPipe[0]);
    close(ServerPipe[1]);
    printspecial(0, "PID that i got was %d\n", pid);
    sprintf(InputDataBuffer, "sudo renice -n -10 -p %d", pid);
    system(InputDataBuffer);
    printspecial(0, "Creating socket\n");
    ServerSocketFileDiscriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocketFileDiscriptor < 0) {
        printspecial(0, "Error opening Socket\n");
        return 1;
    }

    if (argc > 1) {
        sscanf(argv[1], "%d", &PortNumber);
    }
    printspecial(0, "Using port number %d\n", PortNumber);
    bzero((char *) &ServerAddress, sizeof (ServerAddress));
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(PortNumber);
    ServerAddress.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(ServerSocketFileDiscriptor, SOL_SOCKET, SO_REUSEADDR, &ReUseSocket, sizeof (ReUseSocket)) == 1) {
        printspecial(0, "setsockopt");
        return 1;
    }
    if (bind(ServerSocketFileDiscriptor, (struct sockaddr *) &ServerAddress, sizeof (ServerAddress)) < 0) {
        printspecial(0, "Error on binding\n");
        return 1;
    }
    listen(ServerSocketFileDiscriptor, 5);
    int totaltransmits = 0, inputid = -2;
    write(InputPipe[1], &data, sizeof (data));
    while (1) {
        data = 0;
        inputid = -2;
        while (1) {
            Timer_Variable.tv_sec = 0;
            Timer_Variable.tv_usec = 10;
            FD_ZERO(&ReadFileDiscriptors);
            FD_SET(ServerPipe[0], &ReadFileDiscriptors);
            MaximumFileDiscriptorID = ServerPipe[0];
            if (select(MaximumFileDiscriptorID + 1, &ReadFileDiscriptors, NULL, NULL, &Timer_Variable) == 0) {
                printspecial(0, "%d\n", data);
                break;
            }
            data++;
            read(ServerPipe[0], &location, sizeof (location));
            PreviousTime = micros();
            switch (Server_Status) {
                case SERVER_STATUS_CODE_RUNNING_SIM_SG:
                    if (location < SalimGharNOV) {
                        distance += SalimGharSimValues[location];
                    } else {
                        printspecial(1, "End of sim distance=%f, time=%d,total transmits=%d\n", distance, PreviousTime, totaltransmits);
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                        ResetInput();
                        distance = -2;
                    }
                    break;
                case SERVER_STATUS_CODE_RUNNING_SIM_GR:
                    if (location < GoldRushNOV) {
                        distance += GoldRushSimValues[location];
                    } else {
                        printspecial(1, "End of sim distance=%f, time=%d,total transmits=%d\n", distance, PreviousTime, totaltransmits);
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                        ResetInput();
                        distance = 0;
                    }
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                    if (micros() < OldManVideoTime) {
                        distance = -1;
                    } else {
                        distance = SalimGharIncrement*location;
                        if (distance >= 267000) {
                            printspecial(1, "End of track distance=%f, time=%d,total transmits=%d\n", distance, PreviousTime, totaltransmits);
                            Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                            ResetInput();
                            LoadTrackCount();
                            distance = -2;
                        }
                    }
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                    distance = GoldRushIncrement*location;
                    if (distance >= 100000) {
                        printspecial(1, "End of track distance=%f, time=%d,total transmits=%d\n", distance, PreviousTime, totaltransmits);
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                        ResetInput();
                        LoadTrackCount();
                        distance = 0;
                    }
                    break;
                default:
                    if(location < 0)
                    {
                        location =0;
                    }
                    break;
            }
        }
        if ((distance == -1)&&(micros() > OldManVideoTime)&&((Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG)||(Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG))) {
            distance = 0;
        }
        totaltransmits++;
        Timer_Variable.tv_sec = 0;
        Timer_Variable.tv_usec = 40000;
        FD_ZERO(&ReadFileDiscriptors);
        FD_ZERO(&ExceptFileDiscriptors);
        FD_SET(0, &ReadFileDiscriptors);
        FD_SET(ServerSocketFileDiscriptor, &ReadFileDiscriptors);
        FD_SET(ServerSocketFileDiscriptor, &ExceptFileDiscriptors);
        MaximumFileDiscriptorID = ServerSocketFileDiscriptor;

        bzero(OutputDataBuffer, 256);
        sprintf(OutputDataBuffer, "%d\n", (int) distance);
        sprintf(ConnectedAddresses, " ");
        for (ClientCount = 0; ClientCount < MAXCLIENTS; ClientCount++) {
            if (Clients[ClientCount].IsConnected == 1) {
                FD_SET(Clients[ClientCount].ClientSocketFileDiscriptor, &ReadFileDiscriptors);
                FD_SET(Clients[ClientCount].ClientSocketFileDiscriptor, &ExceptFileDiscriptors);
                if (Clients[ClientCount].ClientSocketFileDiscriptor > MaximumFileDiscriptorID) {
                    MaximumFileDiscriptorID = Clients[ClientCount].ClientSocketFileDiscriptor;
                }
                sprintf(ConnectedAddresses, "%s%d ", ConnectedAddresses, Clients[ClientCount].ClientIP[3]);
            }
        }
        select(MaximumFileDiscriptorID + 1, &ReadFileDiscriptors, NULL, &ExceptFileDiscriptors, &Timer_Variable);

        if (FD_ISSET(0, &ReadFileDiscriptors)) {
            char datac[60];
            scanf("%s", &datac);
            sprintf(InputString, "-(-1)%s", datac);
            inputid = -1;
        } else {
            sprintf(InputString, "");
        }
        if (FD_ISSET(ServerSocketFileDiscriptor, &ReadFileDiscriptors)) {
            //				printspecial(0,"accepting Connection\n");
            for (ClientCount = 0; ClientCount < MAXCLIENTS + 1; ClientCount++) {
                if (Clients[ClientCount].IsConnected == 0) {
                    break;
                } else {
                    //						printspecial(0,"Client point %d is busy %d\n",ClientCount,Clients[ClientCount].IsConnected);
                }
            }

            ConnectedClients++;
            Clients[ClientCount].ClientLength = sizeof (Clients[ClientCount].ClientAddress);
            Clients[ClientCount].ClientSocketFileDiscriptor = accept(ServerSocketFileDiscriptor, (struct sockaddr *) &(Clients[ClientCount].ClientAddress), &(Clients[ClientCount].ClientLength));

            sprintf(Address, "%s", inet_ntoa(Clients[ClientCount].ClientAddress.sin_addr));

            sscanf(Address,
                    "%d.%d.%d.%d",
                    &(Clients[ClientCount].ClientIP[0]),
                    &(Clients[ClientCount].ClientIP[1]),
                    &(Clients[ClientCount].ClientIP[2]),
                    &(Clients[ClientCount].ClientIP[3]));
            printspecial(0,
                    "Accepting Client on %d, with ip address %d.%d.%d.%d\n",
                    ClientCount,
                    Clients[ClientCount].ClientIP[0],
                    Clients[ClientCount].ClientIP[1],
                    Clients[ClientCount].ClientIP[2],
                    Clients[ClientCount].ClientIP[3]);
            Clients[ClientCount].DenyInput=0;
            if (Clients[ClientCount].ClientSocketFileDiscriptor < 0) {
                printspecial(0, "ERROR on accept\n");
            } else {
                if (ClientCount != MAXCLIENTS) {
                    Clients[ClientCount].IsConnected = 1;

                } else {
                    printspecial(0, "cannot Accomodate any more clients\n");
                    close(Clients[ClientCount].ClientSocketFileDiscriptor);
                }
            }
        }


        for (ClientCount = 0; ClientCount < MAXCLIENTS; ClientCount++) {
            if ((Clients[ClientCount].IsConnected != 1))
                continue;
            if (FD_ISSET(Clients[ClientCount].ClientSocketFileDiscriptor, &ExceptFileDiscriptors)) {
                printspecial(0, "Client Closed Connection on Client number %d\n", ClientCount);
                close(Clients[ClientCount].ClientSocketFileDiscriptor);
                Clients[ClientCount].IsConnected == 0;
                ConnectedClients--;
                continue;
            }
            if (FD_ISSET(Clients[ClientCount].ClientSocketFileDiscriptor, &ReadFileDiscriptors)) {
                int errorsoc = 0;
                socklen_t len = sizeof (errorsoc);
                int retval = getsockopt(Clients[ClientCount].ClientSocketFileDiscriptor
                        , SOL_SOCKET
                        , SO_ERROR
                        , &errorsoc
                        , &len);
                if ((retval != 0) || (errorsoc != 0)) {
                    printspecial(0, "Client Closed Connection on Client number %d\n", ClientCount);
                    close(Clients[ClientCount].ClientSocketFileDiscriptor);
                    Clients[ClientCount].IsConnected = 0;
                    ConnectedClients--;
                    continue;
                }
                bzero(InputDataBuffer, 256);
                TranssferCount = read(Clients[ClientCount].ClientSocketFileDiscriptor, InputDataBuffer, 255);
                if (TranssferCount < 1) {
                    printspecial(0, "Client Closed Connection on Client number %d\n", ClientCount);
                    close(Clients[ClientCount].ClientSocketFileDiscriptor);
                    Clients[ClientCount].IsConnected = 0;
                    ConnectedClients--;
                    continue;
                    //						printspecial(0,"ERROR reading from socket\n");
                }
#ifdef DEBUG_MODE
                sprintf(InputString + strlen(InputString), "-(%d)%s", ClientCount, InputDataBuffer);
                inputid = ClientCount;
#else               
                if (Clients[ClientCount].ClientIP[0] == 2) {
                    sprintf(InputString, "%s-(%d)%s", InputString, ClientCount, InputDataBuffer);
                    inputid = ClientCount;
                }
#endif
                printspecial(0, "Here is the message received from cleint at ip %d : %s\n", Clients[ClientCount].ClientIP[3], InputDataBuffer);

            }

            int time1, time2;
            time1 = micros();
            if(Clients[ClientCount].DenyInput==0){
                TranssferCount = write(Clients[ClientCount].ClientSocketFileDiscriptor, OutputDataBuffer, strlen(OutputDataBuffer));
            }
            if(Clients[ClientCount].ResponseAvailable){
                TranssferCount = write(Clients[ClientCount].ClientSocketFileDiscriptor, Clients[ClientCount].Response, strlen(Clients[ClientCount].Response));
                Clients[ClientCount].ResponseAvailable=0;
            }
            time2 = micros();
            if ((time2 - time1) > SIMULATION_DECIMATION_TIME) {
                printspecial(1, "This just sucks\n");
            }
            if (TranssferCount < 0) {
                printspecial(0, "ERROR writing to socket disconnecting client %d\n", ClientCount);
            }
        }
        if (inputid == -2) {
            continue;
        }
        int cp;
        printspecial(0, "%s\n", InputString);
        for (cp = 0; cp < strlen(InputString); cp++) {
            char datac = InputString[cp], Response[100];
            if (datac == '-') {
                sscanf(InputString + cp, "-(%d)", &inputid);
                while (InputString[cp] != ')') {
                    cp++;
                }
                // printspecial(2,"Identified %d as input\n",inputid);
                continue;
            } else if (datac == 'S') {
                sprintf(Response, "Closing Server\n");
                for (ClientCount = 0; ClientCount < MAXCLIENTS; ClientCount++) {
                    if (Clients[ClientCount].IsConnected == 1) {
                        close(Clients[ClientCount].ClientSocketFileDiscriptor);
                    }
                }
                close(ServerSocketFileDiscriptor);
                kill(pid, SIGQUIT);
                delay(10);
                return;
            } else if (datac == 'R') {
                switch (Server_Status) {
                    case SERVER_STATUS_CODE_RUNNING_SIM_GR:
                        distance = 0;
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                        sprintf(Response, "Resetting count and distance and running Gold rush simulation\n");
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_SIM_SG:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                        sprintf(Response, "Resetting count and distance and Running to Salimghar simulation\n");
                        distance = -2;
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                        distance = 0;
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                        sprintf(Response, "Resetting count and distance and running Gold rush simulation\n");
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                        sprintf(Response, "Resetting count and distance and Running to Salimghar simulation\n");
                        distance = -2;
                        ResetInput();
                        break;
                    default:
                        sprintf(Response, "Nothing to do, currently in reset mode\n");
                        break;
                }
            } else if (datac == 'G') {
                sprintf(Response, "%d Clients Connected with IP's%s\n", ConnectedClients, ConnectedAddresses);
            } else if (datac == 'P') {
                if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR) {
                    sprintf(Response, "SIM GR S\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET) {
                    sprintf(Response, "SIM GR R\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET) {
                    sprintf(Response, "SIM SG R\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG) {
                    sprintf(Response, "SIM SG S\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR) {
                    sprintf(Response, "RUN GR S\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET) {
                    sprintf(Response, "RUN GR R\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET) {
                    sprintf(Response, "RUN SG R\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG) {
                    sprintf(Response, "RUN SG S\n");
                } else {
                    sprintf(Response, "FL\n");
                }
            } else if (datac == 'L') {
                if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG;
                    distance = -1;
                    ResetInput();
                    OldManVideoTime = micros();
                    OldManVideoTime += 17000000;
                    sprintf(Response, "Launching oldman video\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR;
                    distance = 0;
                    ResetInput();
                    sprintf(Response, "Started GoldRush\n");
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG;
                    distance = -1;
                    ResetInput();
                    OldManVideoTime = micros();
                    OldManVideoTime += 17000000;
                    location = 0;
                    sprintf(Response, "Launching oldman video %d %d\n", micros(), OldManVideoTime);
                } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR;
                    distance = 0;
                    ResetInput();
                    sprintf(Response, "Started GoldRush\n");
                } else {
                    sprintf(Response, "Wrong server_status for launch\n");
                }
            } else if (datac == 'W') {

                switch (Server_Status) {
                    case SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET:
                    case SERVER_STATUS_CODE_RUNNING_SIM_SG:
                        distance = 0;
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                        sprintf(Response, "Resetting count and distance and switching to Gold rush simulation\n");
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET:
                    case SERVER_STATUS_CODE_RUNNING_SIM_GR:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                        sprintf(Response, "Resetting count and distance and switching to Salimghar simulation\n");
                        distance = -2;
                        ResetInput();
                        break;

                    case SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET:
                    case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                        distance = 0;
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                        sprintf(Response, "Resetting count and distance and switching to GoldRush run mode\n");
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET:
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                        sprintf(Response, "Resetting count and distance and switching to Salimghar run mode\n");
                        distance = -2;
                        ResetInput();
                        break;
                    default:
                        break;
                }

            } else if (datac == 'V') {

                switch (Server_Status) {
                    case SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET:
                    case SERVER_STATUS_CODE_RUNNING_SIM_SG:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                        sprintf(Response, "Resetting count and distance and switching to Salimghar run mode\n");
                        ResetInput();
                        distance = -2;
                        break;
                    case SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET:
                    case SERVER_STATUS_CODE_RUNNING_SIM_GR:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                        sprintf(Response, "Resetting count and distance and switching to GoldRush run mode\n");
                        distance = 0;
                        ResetInput();
                        break;

                    case SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET:
                    case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                        distance = -2;
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                        sprintf(Response, "Resetting count and distance and switching to Salimghar simulation\n");
                        ResetInput();
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET:
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                        sprintf(Response, "Resetting count and distance and switching to GoldRush simulation\n");
                        distance = 0;
                        ResetInput();
                        break;
                    default:
                        break;
                }

            } else if(datac == 'D')
            {
                if ((inputid>-1)&&(inputid < MAXCLIENTS)) {
                    if (Clients[inputid].DenyInput) {
                        sprintf(Response, "Measurement Transmission started\n");
                        Clients[inputid].DenyInput = 0;
                    }else
                    {
                        sprintf(Response, "Measurement stopped\n");
                        Clients[inputid].DenyInput = 1;
                    }
                } else
                {
                    sprintf(Response,"Deny measurement response is for clients only %d\n",inputid);
                }
            }else if (datac == 'I') {
                sprintf(Response, "Count=%d,distance=%f\n", location, distance);
            } else {
                // printspecial(2,"Don't know how you came here %c %d\n",datac,datac);
                continue;
            }
            switch (inputid) {
                case -2:
                    printspecial(3, "no known input source %s", Response);
                    break;
                case -1:
                    printspecial(1, "%s", Response);
                    break;
                default:
                    if ((inputid < 0) || (inputid > MAXCLIENTS)||(Clients[inputid].IsConnected==0)) {
                        printspecial(3, "input id %d has to receive\n%s", inputid, Response);
                    } else {
                        if (Clients[inputid].ResponseAvailable) {
                            sprintf(Clients[inputid].Response+strlen(Clients[inputid].Response), "%s", Response);
                        }else
                        {
                            sprintf(Clients[inputid].Response, "%s", Response);
                        }
                        Clients[inputid].ResponseAvailable=1;
                    }
                    break;
            }
        }
    }

}



