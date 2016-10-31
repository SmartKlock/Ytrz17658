#include <stdarg.h>
#include <signal.h>
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

#include <unistd.h>
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
#define SERVER_STATUS_CODE_RUNNING_SIM_GR        3
#define SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET  2
#define SERVER_STATUS_CODE_RUNNING_RUN_GR        11
#define SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET  10
#define SERVER_STATUS_CODE_RUNNING_SIM_SG        5
#define SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET  4
#define SERVER_STATUS_CODE_RUNNING_RUN_SG        13
#define SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET  12
#define SERVER_STATUS_CODE_RUNNING_SIM_MM        7
#define SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET  6
#define SERVER_STATUS_CODE_RUNNING_RUN_MM        15
#define SERVER_STATUS_CODE_RUNNING_RUN_MM_RESET  14
#define SERVER_STATUS_CODE_RESET_MODE            1

#define PREFERRED_WORKING_DIRECTORY              ""

#define GR_SIMULATION_FILE_PATH                 "goldrushvalues.txt"
#define GR_TRACK_FILE_PATH                      "GoldRushTrackCount.txt"
#define MM_SIMULATION_FILE_PATH                 "mitmvalues.txt"
#define MM_TRACK_FILE_PATH                      "MITMTrackCount.txt"
#define SG_SIMULATION_FILE_PATH                 "salimgharvalues.txt"
#define SG_TRACK_FILE_PATH                      "SalimGharTrackCount.txt"



#define SIMULATION_DECIMATION_TIME          4000
#define INTERRUPT_MODE

#define SERVER_STATUS_CODE_TYPE_RUN_RESET       8
#define SERVER_STATUS_CODE_TYPE_RUN_STARTED     9
#define SERVER_STATUS_CODE_TYPE_SIM_RESET       0
#define SERVER_STATUS_CODE_TYPE_SIM_STARTED     1


#define APP_COMMAND_TYPE_SHUTDOWN_SERVER            'E'
#define APP_COMMAND_TYPE_STOP_SERVER                'S'
#define APP_COMMAND_TYPE_GET_STATUS                 'P'
#define APP_COMMAND_TYPE_GET_LOCATION               'I'
#define APP_COMMAND_TYPE_GET_CONNECTED_CLIENTS      'G'
#define APP_COMMAND_TYPE_RESET_EXPERIENCE           'R'
#define APP_COMMAND_TYPE_START_EXPERIENCE           'L'
#define APP_COMMAND_TYPE_SWITCH_EXPERIENCE          'W'
#define APP_COMMAND_TYPE_SWITCH_REALITY             'V'
#define APP_COMMAND_TYPE_DISABLE_MEASUREMENT        'D'
#define APP_COMMAND_TYPE_CALIBRATE_READING          'C'
#define APP_COMMAND_TYPE_LOAD_CALIBRATION           'A'

#define APP_COMMAND_STRING_SHUTDOWN_SERVER            "SHUTDOWN"
#define APP_COMMAND_STRING_STOP_SERVER                "EXIT"
#define APP_COMMAND_STRING_GET_STATUS                 "SETTING"
#define APP_COMMAND_STRING_GET_LOCATION               "LOCATION"
#define APP_COMMAND_STRING_GET_CONNECTED_CLIENTS      "CLIENTS"
#define APP_COMMAND_STRING_RESET_EXPERIENCE           "RESET"
#define APP_COMMAND_STRING_START_EXPERIENCE           "START"
#define APP_COMMAND_STRING_DISABLE_MEASUREMENT        "DBM"
#define APP_COMMAND_STRING_CALIBRATE_READING          "CALIBRATE"
#define APP_COMMAND_STRING_LOAD_CALIBRATION           "LOAD"
#define APP_COMMAND_STRING_STORE_BATTERY              "BATTERY"
#define APP_COMMAND_STRING_STORE_TEMPERATURE          "TEMPERATURE"
#define APP_COMMAND_STRING_STORE_PROXIMITY            "PROXIMITY"
#define APP_COMMAND_STRING_STORE_STATUS               "STATUS"

struct ClientStatus {
    int IsConnected;
    pthread_t ThreadId;
    int ClientSocketFileDiscriptor, ClientLength;
    struct sockaddr_in ClientAddress;
    int ClientIP[4], ResponseAvailable, DenyInput;
    float Temperature, Battery_Percentage;
    int Proximity, Status;
    char *Response,ResponseBuffer[2000], Command[200];
};

int Server_Status = SERVER_STATUS_CODE_INIT;

void ProcessCommand(int ClientCount);

int StateIsType(int type) {
    return ((Server_Status & 0x09) == (type & 0x09));
}

int ThisStateIsType(int ThisStat, int Type) {
    return ((ThisStat & 0x09) == (Type & 0x09));
}

int MITMSimValues[150000], MITMNOV, MITMTrackCount = 2335;
float MITMIncrement = 0, MITMTrackLength = 100000.0;

float GoldRushIncrement = 0, GoldRushTrackLength = 840000.0;
int GoldRushSimValues[70000], GoldRushNOV, GoldRushTrackCount = 4000;

int SalimGharSimValues[150000], SalimGharNOV, SalimgharTrackCount = 8750;
float SalimGharIncrement = 0, SalimGharTrackLength = 267000.0;

int InputPinLog[4][150000], InputPinLogPointer[4], InputPinLogPointerPrevious[4];
char LogFilePath[30], pwd[30];

float distance;
int TranssferCount, OldManVideoTime = 0;

struct sigaction InputSigAction, ServerSigAction;
int ServerSocketFileDiscriptor, PortNumber = 1234, ClientCount = 0, MaximumFileDiscriptorID, ReUseSocket = 1, ConnectedClients = 0;
int SimulationStartTime, RunStartTime = 0, PreviousTransmitTime, PreviousTime, PreviousLocation = 0, location;
struct sockaddr_in ServerAddress;
struct timeval Timer_Variable;
struct itimerval ServerTimer, InputTimer;
struct ClientStatus Clients[MAXCLIENTS + 1], FileCheckStruct;
fd_set ReadFileDiscriptors, ExceptFileDiscriptors;
char InputDataBuffer[256], OutputDataBuffer[256], Address[20], ConnectedAddresses[500];

int stopeverything = 0, servercount = 0, inputcount = 0, connectedVR = 0;

int InputPipe[2], InputThreadProcessId, ServerPipe[2];

void InputCounter1(void) {
    InputPinLogPointer[0]++;
    InputPinLog[0][InputPinLogPointer[0]] = micros() - RunStartTime;
}

void InputCounter2(void) {
    InputPinLogPointer[1]++;
    InputPinLog[1][InputPinLogPointer[1]] = micros() - RunStartTime;
}

void InputCounter3(void) {
    InputPinLogPointer[2]++;
    InputPinLog[2][InputPinLogPointer[2]] = micros() - RunStartTime;
}

void InputCounter4(void) {
    InputPinLogPointer[3]++;
    InputPinLog[3][InputPinLogPointer[3]] = micros() - RunStartTime;
}

void printspecial(int priority, char *format, ...) {
    FILE *Writer;
    va_list arg;
    va_start(arg, *format);
    Writer = fopen(LogFilePath, "a+");
    if (priority > -1)
        vfprintf(Writer, format, arg);
    va_end(arg);
    fclose(Writer);
}

void LogRunValues(char* heading, int logtype) {
    int iterator1, iterator2;
    FILE *Writer;
    Writer = fopen(LogFilePath, "a+");
    fprintf(Writer, "%s", heading);
    for (iterator1 = 0; iterator1 < logtype; iterator1++) {
        fprintf(Writer, "Senor Log For Sensor %d", iterator1 + 1);
        for (iterator2 = 0; iterator2 < InputPinLogPointer[iterator1]; iterator2++) {
            fprintf(Writer, ",%d", InputPinLog[iterator1][iterator2]);
        }
        fprintf(Writer, "\n");
    }
    fclose(Writer);
}

void InitLogFile(void) {
    FILE *Reader, *Writer;
    int lognumber = 0;
    if (stat("log.txt", &FileCheckStruct) == 0) {
        Reader = fopen("log.txt", "r");
        fscanf(Reader, "%d\n", &lognumber);
        fclose(Reader);
    } else {
        system("mkdir log");
        lognumber = 0;
    }
    sprintf(LogFilePath, "log/log%d.txt", lognumber);
    printspecial(1, "Starting Log File %d\n", lognumber++);
    Writer = fopen("log.txt", "w");
    fprintf(Writer, "%d\n", lognumber);
    fclose(Writer);
}

void sigquit() {
    printspecial(0, "My DADDY has Killed me!!!\n");
    stopeverything = 1;
}

void ResetInput() {
    int sending = 0;
    write(InputPipe[1], &Server_Status, sizeof (Server_Status));
    InputPinLogPointer[0] = 0;
    InputPinLogPointer[1] = 0;
    InputPinLogPointer[2] = 0;
    InputPinLogPointer[3] = 0;
    location = 0;
    printspecial(0, "Resetting input\n");
    while (location != -10) {

        Timer_Variable.tv_sec = 0;
        Timer_Variable.tv_usec = 100;
        FD_ZERO(&ReadFileDiscriptors);
        FD_SET(ServerPipe[0], &ReadFileDiscriptors);
        MaximumFileDiscriptorID = ServerPipe[0];
        if (select(MaximumFileDiscriptorID + 1, &ReadFileDiscriptors, NULL, NULL, &Timer_Variable) == 0) {
            //            printspecial(0, "Waiting on sub thread%d\n", sending++);
            continue;
        }
        read(ServerPipe[0], &location, sizeof (location));
    }
    location = 0;
}

void LoadTrackCount() {
    FILE *Reader;
    printspecial(11, "Loading track count\n");
    if (stat(SG_TRACK_FILE_PATH, &FileCheckStruct) == 0) {
        Reader = fopen(SG_TRACK_FILE_PATH, "r");
        fscanf(Reader, "%d\n", &SalimgharTrackCount);
        fscanf(Reader, "%f\n", &SalimGharTrackLength);
        fclose(Reader);
        if (SalimgharTrackCount < 1) {
            SalimgharTrackCount = 8750;
        }
    }

    SalimGharIncrement = SalimGharTrackLength / (float) SalimgharTrackCount;

    if (stat(GR_TRACK_FILE_PATH, &FileCheckStruct) == 0) {
        Reader = fopen(GR_TRACK_FILE_PATH, "r");
        fscanf(Reader, "%d\n", &GoldRushTrackCount);
        fscanf(Reader, "%f\n", &GoldRushTrackLength);
        fclose(Reader);
        if (GoldRushTrackCount < 1) {
            GoldRushTrackCount = 4000;
        }
    }
    GoldRushIncrement = GoldRushTrackLength / (float) GoldRushTrackCount;


    if (stat(MM_TRACK_FILE_PATH, &FileCheckStruct) == 0) {
        Reader = fopen(MM_TRACK_FILE_PATH, "r");
        fscanf(Reader, "%d\n", &MITMTrackCount);
        fscanf(Reader, "%f\n", &MITMTrackLength);
        fclose(Reader);
        if (MITMTrackCount < 1) {
            MITMTrackCount = 2335;
        }
    }
    MITMIncrement = MITMTrackLength / (float) MITMTrackCount;
}

void StoreTrackCount() {
    FILE *Writer;
    printspecial(11, "storing track count\n");
    Writer = fopen(SG_TRACK_FILE_PATH, "w");
    fprintf(Writer, "%d\n", SalimgharTrackCount);
    fprintf(Writer, "%f\n", SalimGharTrackLength);
    fclose(Writer);
    if (SalimgharTrackCount < 1) {
        SalimgharTrackCount = 8750;
    }

    Writer = fopen(GR_TRACK_FILE_PATH, "w");
    fprintf(Writer, "%d\n", GoldRushTrackCount);
    fprintf(Writer, "%f\n", GoldRushTrackLength);
    fclose(Writer);
    if (GoldRushTrackCount < 1) {
        GoldRushTrackCount = 4000;
    }


    Writer = fopen(MM_TRACK_FILE_PATH, "w");
    fprintf(Writer, "%d\n", MITMTrackCount);
    fprintf(Writer, "%f\n", MITMTrackLength);
    fclose(Writer);
    if (MITMTrackCount < 1) {
        MITMTrackCount = 4000;
    }
    LoadTrackCount();
}

int ReadSimValues(char *path, int *SimValues) {
    int length;
    long iterator1;
    FILE *Reader;
    if (stat(path, &FileCheckStruct) == 0) {
        printspecial(0, "Reading Sim Values from %s\n", path);
        Reader = fopen(path, "r");
        fscanf(Reader, "%d\n", &length);
        for (iterator1 = 0; iterator1 < length; iterator1++) {
            fscanf(Reader, "%d\n", (SimValues + iterator1));
        }
        fclose(Reader);
        printspecial(0, "The length was found to be %d\n", length);
        return length;
    } else {
        printspecial(0, "cound not find sim values %s\n", path);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    char InputString[600];
    int iterator;
    delay(3000);
    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;

    for (iterator = 1; iterator < argc; iterator++) {
        if (strcmp(argv[iterator], "-pwd") == 0) {
            chdir(argv[++iterator]);
        } else if (strcmp(argv[iterator], "-portnumber") == 0) {
            iterator++;
            sscanf(argv[iterator], "%d", &PortNumber);
        } else if (strcmp(argv[iterator], "-RUN") == 0) {
            iterator++;
            if (strcmp(argv[iterator], "GR") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                distance = 0;
            } else if (strcmp(argv[iterator], "SG") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                distance = -2;
            } else if (strcmp(argv[iterator], "MM") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_MM_RESET;
                distance = 0;
            }
        } else if (strcmp(argv[iterator], "-SIM") == 0) {
            iterator++;
            if (strcmp(argv[iterator], "GR") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                distance = 0;
            } else if (strcmp(argv[iterator], "SG") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                distance = -2;
            } else if (strcmp(argv[iterator], "MM") == 0) {
                Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET;
                distance = 0;
            }
        }
    }
    InitLogFile();
    printspecial(0, "reading goldrush values\n");
    GoldRushNOV = ReadSimValues(GR_SIMULATION_FILE_PATH, GoldRushSimValues);

    printspecial(0, "reading salimghar values\n");
    SalimGharNOV = ReadSimValues(SG_SIMULATION_FILE_PATH, SalimGharSimValues);

    printspecial(0, "reading MM values\n");
    MITMNOV = ReadSimValues(MM_SIMULATION_FILE_PATH, MITMSimValues);

    LoadTrackCount();

    printspecial(0, "Clearing clients\n");
    for (ClientCount = 0; ClientCount < MAXCLIENTS + 1; ClientCount++) {
        Clients[ClientCount].IsConnected = 0;
        Clients[ClientCount].DenyInput = 0;
        Clients[ClientCount].Battery_Percentage=0;
        Clients[ClientCount].Temperature=0;
        Clients[ClientCount].Proximity=0;
        Clients[ClientCount].Status=0;
        sprintf(Clients[ClientCount].Command,"AT");
    }
    printspecial(0, "Starting Simulation\n");

    printspecial(0, "forking\n");

    int data = 0;
    pipe(InputPipe);
    pipe(ServerPipe);
    InputThreadProcessId = fork();
    if (InputThreadProcessId == 0) {
        int CurrentTime, sucks = 0;
        char Command[50];
        close(InputPipe[1]);
        close(ServerPipe[0]);

        if (wiringPiSetup() == -1) {
            printspecial(0, "Wiring Pi did not load\n");
            return 1;
        }
        pinMode(INPUTPIN1, INPUT);
        pinMode(INPUTPIN2, INPUT);
        pinMode(INPUTPIN3, INPUT);
        pinMode(INPUTPIN4, INPUT);
        sprintf(Command, "gpio mode %d down", INPUTPIN1);
        system(Command);
        sprintf(Command, "gpio mode %d down", INPUTPIN2);
        system(Command);
        sprintf(Command, "gpio mode %d down", INPUTPIN3);
        system(Command);
        sprintf(Command, "gpio mode %d down", INPUTPIN4);
        system(Command);
        InputPinLogPointer[0] = 0;
        InputPinLogPointer[1] = 0;
        InputPinLogPointer[2] = 0;
        InputPinLogPointer[3] = 0;
        if (wiringPiISR(INPUTPIN1, INT_EDGE_BOTH, &InputCounter1) != 0) {
            printspecial(0, "Wiring Pi isr did not load on pin 1\n");
            return 1;
        }
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
        }
        printspecial(0, "forked into child\n");
        read(InputPipe[0], &data, sizeof (data));
        SimulationStartTime = micros();
        PreviousTime = SimulationStartTime - SIMULATION_DECIMATION_TIME;
        PreviousTransmitTime = SimulationStartTime - 50000;
        while (1) {
            CurrentTime = micros();
            if (StateIsType(SERVER_STATUS_CODE_TYPE_SIM_STARTED)) {
                if ((CurrentTime - PreviousTime) > SIMULATION_DECIMATION_TIME) {
                    location = (CurrentTime - SimulationStartTime) / SIMULATION_DECIMATION_TIME; //&&(( CurrentTime - SimulationStartTime )%SIMULATION_DECIMATION_TIME)<1000){
                    if ((location - PreviousLocation) > 1) {
                        sucks += (location - PreviousLocation);
                        printspecial(1, "Simulation lagged behind %d,timediff =%d\n", sucks, CurrentTime - PreviousTransmitTime);
                        PreviousTransmitTime = CurrentTime;
                    }
                    PreviousTime = (SIMULATION_DECIMATION_TIME * location) + SimulationStartTime;
                    PreviousLocation = location;
                    write(ServerPipe[1], &location, sizeof (location));
                }
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR) {
                location = (InputPinLogPointer[0] +
                        InputPinLogPointer[1] +
                        InputPinLogPointer[2] +
                        InputPinLogPointer[3]);
                if (location != PreviousLocation) {
                    write(ServerPipe[1], &location, sizeof (location));
                    //                    printspecial(3, "location changed to %d\n", location);
                }
                PreviousLocation = location;
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_MM) {
                location = InputPinLogPointer[0];
                if (location != PreviousLocation) {
                    //                    printspecial(3, "location changed to %d\n", location);
                    write(ServerPipe[1], &location, sizeof (location));
                }
                PreviousLocation = location;
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG) {
                location = InputPinLogPointer[0];
                if (location != PreviousLocation) {
                    //                    printspecial(3, "location changed to %d\n", location);
                    write(ServerPipe[1], &location, sizeof (location));
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
                switch (prevserver) {
                    case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                        LogRunValues("Gold Rush Run Values\n", 4);
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                        LogRunValues("Salimghar Run Values\n", 1);
                        break;
                    case SERVER_STATUS_CODE_RUNNING_RUN_MM:
                        LogRunValues("MITM Run Values\n", 4);
                        break;
                    default:
                        break;
                }
                //                StoreTrackCount();
                if ((StateIsType(SERVER_STATUS_CODE_TYPE_RUN_STARTED))&&(ThisStateIsType(prevserver, SERVER_STATUS_CODE_TYPE_RUN_RESET))) {
                    InputPinLogPointer[0] = 0;
                    InputPinLogPointer[1] = 0;
                    InputPinLogPointer[2] = 0;
                    InputPinLogPointer[3] = 0;
                    RunStartTime = micros();
                    /* Attention remember top write code to store the logs*/
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
    printspecial(0, "PID that i got was %d\n", InputThreadProcessId);
    system(InputDataBuffer);
    printspecial(0, "Creating socket\n");
    ServerSocketFileDiscriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocketFileDiscriptor < 0) {
        printspecial(0, "Error opening Socket\n");
        return 1;
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
                case SERVER_STATUS_CODE_RUNNING_SIM_MM:
                    if (location < MITMNOV) {
                        distance += MITMSimValues[location];
                    } else {
                        printspecial(1, "End of sim distance=%f, time=%d,total transmits=%d\n", distance, PreviousTime, totaltransmits);
                        Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET;
                        ResetInput();
                        distance = 0;
                    }
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                    if (micros() < OldManVideoTime) {
                        distance = -1;
                    } else {
                        distance = SalimGharIncrement*location;

                    }
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                    distance = GoldRushIncrement*location;
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_MM:
                    distance = MITMIncrement*location;
                    break;
                default:
                    if (location < 0) {
                        location = 0;
                    }
                    break;
            }
        }
        if ((distance == -1)&&(micros() > OldManVideoTime)&&((Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG) || (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG))) {
            distance = 0;
        }
        connectedVR = 0;
        totaltransmits++;
        Timer_Variable.tv_sec = 0;
        Timer_Variable.tv_usec = 40000;
        FD_ZERO(&ReadFileDiscriptors);
        FD_ZERO(&ExceptFileDiscriptors);
        //        FD_SET(0, &ReadFileDiscriptors);
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
                if (Clients[ClientCount].DenyInput == 0) {
                    sprintf(ConnectedAddresses, "%sIP=%d:%d:%d:%d Battery=%f Temperature=%f Status=%d Proximity=%d\n ", ConnectedAddresses, Clients[ClientCount].ClientIP[0], Clients[ClientCount].ClientIP[1], Clients[ClientCount].ClientIP[2], Clients[ClientCount].ClientIP[3],, Clients[ClientCount].Battery_Percentage, Clients[ClientCount].Temperature, Clients[ClientCount].Status, Clients[ClientCount].Proximity);
                    connectedVR++;
                }
            }
        }
        select(MaximumFileDiscriptorID + 1, &ReadFileDiscriptors, NULL, &ExceptFileDiscriptors, &Timer_Variable);

        if (FD_ISSET(0, &ReadFileDiscriptors)) {
            char datac[60];
            scanf("%s", &datac);
        } else {
            sprintf(InputString, "");
        }
        if (FD_ISSET(ServerSocketFileDiscriptor, &ReadFileDiscriptors)) {
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
            Clients[ClientCount].DenyInput = 0;
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
                }
#ifdef DEBUG_MODE
                sprintf(Clients[ClientCount].Command, "%s", InputDataBuffer);
                //                sprintf(InputString + strlen(InputString), "-(%d)%s", ClientCount, InputDataBuffer);
                //                inputid = ClientCount;
#else               
                if (Clients[ClientCount].ClientIP[0] == 2) {
                    sprintf(InputString, "%s-(%d)%s", InputString, ClientCount, InputDataBuffer);
                    inputid = ClientCount;
                }
#endif
                printspecial(0, "Message received from ip %d with client id %d : %s\n", Clients[ClientCount].ClientIP[3], ClientCount, InputDataBuffer);

            }

            int time1, time2;
            char OutData[500];
            bzero(OutData, 500);
            time1 = micros();
            if (Clients[ClientCount].DenyInput == 0) {
                sprintf(OutData, "%s", OutputDataBuffer);
            }
            if (Clients[ClientCount].ResponseAvailable) {
                sprintf(OutData, "%s%s", OutData, Clients[ClientCount].Response);
                Clients[ClientCount].ResponseAvailable = 0;
            }
            TranssferCount = write(Clients[ClientCount].ClientSocketFileDiscriptor, OutData, strlen(OutData));
            time2 = micros();
            if ((time2 - time1) > SIMULATION_DECIMATION_TIME) {
                printspecial(1, "Data writing took too much time\n");
            }
            if (TranssferCount < 0) {
                printspecial(0, "Error writing to client %d\n", ClientCount);
            }
        }
//        if (inputid == -2) {
//            continue;
//        }
        for(ClientCount=0;ClientCount<MAXCLIENTS;ClientCount++)
        {   
            if(strcmp(Clients[ClientCount].Command,"AT")!=0)
            {
                ProcessCommand(ClientCount);
                sprintf(Clients[ClientCount].Command,"AT");
            }
        }
//        ProcessCommand(InputString);
    }

}

void ProcessCommand(int ClientCount)
{
    int i=0;
    char Command[50],Parameters[2][50];
    Clients[ClientCount].ResponseAvailable=1;
    Clients[ClientCount].Response=Clients[ClientCount].ResponseBuffer;
    while(i<strlen(Clients[ClientCount].Command))
    {
        sprintf(Clients[ClientCount].Response,"Please Clarify\n");
        sscanf(Clients[ClientCount].Command+i,"%s",Command);
        i+=strlen(Command)+1;
        if(strcmp(Command,APP_COMMAND_STRING_STOP_SERVER)==0)
        {
            write(Clients[ClientCount].ClientSocketFileDiscriptor, "Stopping Server\n",strlen("Stopping Server\n"));
            for (ClientCount = 0; ClientCount < MAXCLIENTS; ClientCount++) {
                if (Clients[ClientCount].IsConnected == 1) {
                    close(Clients[ClientCount].ClientSocketFileDiscriptor);
                }
            }
            close(ServerSocketFileDiscriptor);
            if (StateIsType(SERVER_STATUS_CODE_TYPE_RUN_STARTED)) {
                Server_Status &= 0x0E;
                ResetInput();
                delay(500);
            }
            kill(InputThreadProcessId, SIGQUIT);
            printspecial(0, "Shutting down the server\n");
            delay(10);
            exit(0);
        }
        else if(strcmp(Command,APP_COMMAND_STRING_SHUTDOWN_SERVER)==0)
        {
            write(Clients[ClientCount].ClientSocketFileDiscriptor, "Shutting Down The Server\n",strlen("Shutting Down The Server\n"));
            for (ClientCount = 0; ClientCount < MAXCLIENTS; ClientCount++) {
                if (Clients[ClientCount].IsConnected == 1) {
                    close(Clients[ClientCount].ClientSocketFileDiscriptor);
                }
            }
            close(ServerSocketFileDiscriptor);
            if (StateIsType(SERVER_STATUS_CODE_TYPE_RUN_STARTED)) {
                Server_Status &= 0x0E;
                ResetInput();
                delay(500);
            }
            printspecial(0, "Shutting down the Pi\n");
            kill(InputThreadProcessId, SIGQUIT);
            delay(10);
            system("sudo shutdown -h now");
            exit(0);
        }
        else if(strcmp(Command,APP_COMMAND_STRING_RESET_EXPERIENCE)==0)
        {
            if (i <= strlen(Clients[ClientCount].Command)) {
                sscanf(Clients[ClientCount].Command + i, "%s", Parameters[0]);
                i += strlen(Parameters[0]) + 1;
                if (i <= strlen(Clients[ClientCount].Command)) {
                    sscanf(Clients[ClientCount].Command + i, "%s", Parameters[1]);
                    i += strlen(Parameters[1]) + 1;
                }
            }
            if (strcmp(Parameters[0], "SIM")==0) {
                if (strcmp(Parameters[1], "GR")==0) {
                    distance = 0;
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running Gold rush simulation\n");
                    ResetInput();
                } else if (strcmp(Parameters[1], "MM")==0) {
                    distance = 0;
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running MITM simulation\n");
                    ResetInput();
                } else if (strcmp(Parameters[1], "SG")==0) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running SalinGhar simulation\n");
                    distance = -2;
                    ResetInput();
                }
            } else if (strcmp(Parameters[0], "RUN")==0) {
                if (strcmp(Parameters[1], "GR")==0) {
                    distance = 0;
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running Gold rush sensor\n");
                    ResetInput();
                } else if (strcmp(Parameters[1], "MM")==0) {
                    distance = 0;
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_MM_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running MITM sensor\n");
                    ResetInput();
                } else if (strcmp(Parameters[1], "SG")==0) {
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET;
                    sprintf(Clients[ClientCount].Response, "Resetting count and distance and running SalimGhar sensor\n");
                    distance = -2;
                    ResetInput();
                }
            }
        }
        else if(strcmp(Command,APP_COMMAND_STRING_START_EXPERIENCE)==0)
        {
            switch (Server_Status) {
                case SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_GR;
                    distance = 0;
                    ResetInput();
                    sprintf(Clients[ClientCount].Response, "Started GoldRush simulation\n");
                    break;
                case SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_SG;
                    ResetInput();
                    OldManVideoTime=micros();
                    OldManVideoTime+=17000000;
                    distance = -1;
                    sprintf(Clients[ClientCount].Response, "Started Salimghar simulation\n");
                    break;
                case SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_SIM_MM;
                    distance = 0;
                    ResetInput();
                    sprintf(Clients[ClientCount].Response, "Started MITM simulation\n");
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_GR;
                    distance = 0;
                    ResetInput();
                    sprintf(Clients[ClientCount].Response, "Started GoldRush sensor\n");
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_SG;
                    ResetInput();
                    OldManVideoTime=micros();
                    OldManVideoTime+=17000000;
                    distance = -1;
                    sprintf(Clients[ClientCount].Response, "Started Salimghar sensor\n");
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_MM_RESET:
                    Server_Status = SERVER_STATUS_CODE_RUNNING_RUN_MM;
                    distance = 0;
                    ResetInput();
                    sprintf(Clients[ClientCount].Response, "Started MITM sensor\n");
                    break;
            }
        }
        else if(strcmp(Command,APP_COMMAND_STRING_GET_STATUS)==0)
        {
            if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR) {
                sprintf(Clients[ClientCount].Response, "SIM GR S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_GR_RESET) {
                sprintf(Clients[ClientCount].Response, "SIM GR R\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR) {
                sprintf(Clients[ClientCount].Response, "RUN GR S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_GR_RESET) {
                sprintf(Clients[ClientCount].Response, "RUN GR R\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG) {
                sprintf(Clients[ClientCount].Response, "SIM SG S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_SG_RESET) {
                sprintf(Clients[ClientCount].Response, "SIM SG R\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG) {
                sprintf(Clients[ClientCount].Response, "RUN SG S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_SG_RESET) {
                sprintf(Clients[ClientCount].Response, "RUN SG R\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_MM) {
                sprintf(Clients[ClientCount].Response, "SIM MM S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_SIM_MM_RESET) {
                sprintf(Clients[ClientCount].Response, "SIM MM R\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_MM) {
                sprintf(Clients[ClientCount].Response, "RUN MM S\n");
            } else if (Server_Status == SERVER_STATUS_CODE_RUNNING_RUN_MM_RESET) {
                sprintf(Clients[ClientCount].Response, "RUN MM R\n");
            } else {
                sprintf(Clients[ClientCount].Response, "FL\n");
            }
        }
        else if(strcmp(Command,APP_COMMAND_STRING_GET_LOCATION)==0)
        {
            sprintf(Clients[ClientCount].Response, "Count=%d,distance=%f\n", location, distance);
        }
        else if(strcmp(Command,APP_COMMAND_STRING_GET_CONNECTED_CLIENTS)==0)
        {
            sprintf(Clients[ClientCount].Response, "%d Clients Connected with IP's%s\n", connectedVR, ConnectedAddresses);
        }
        else if(strcmp(Command,APP_COMMAND_STRING_CALIBRATE_READING)==0)
        {
            switch (Server_Status) {
                case SERVER_STATUS_CODE_RUNNING_RUN_GR:
                    GoldRushTrackCount = location;
                    GoldRushIncrement = GoldRushTrackLength / (float) GoldRushTrackCount;
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_SG:
                    SalimgharTrackCount = location;
                    SalimGharIncrement = SalimGharTrackLength / (float) SalimgharTrackCount;
                    break;
                case SERVER_STATUS_CODE_RUNNING_RUN_MM:
                    MITMTrackCount = location;
                    MITMIncrement = MITMTrackLength / (float) MITMTrackCount;
                    break;
            }
            sprintf(Clients[ClientCount].Response, "Storing Track Count\n Salimghar %d,%f\nGoldRush %d,%f\n MITM %d,%f\n", SalimgharTrackCount, SalimGharTrackLength, GoldRushTrackCount, GoldRushTrackLength, MITMTrackCount, MITMTrackLength);
            StoreTrackCount();
        }
        else if(strcmp(Command,APP_COMMAND_STRING_LOAD_CALIBRATION)==0)
        {
            LoadTrackCount();
            sprintf(Clients[ClientCount].Response, "Loaded Track Count\n Salimghar %d,%f\nGoldRush %d,%f\n MITM %d,%f\n", SalimgharTrackCount, SalimGharTrackLength, GoldRushTrackCount, GoldRushTrackLength, MITMTrackCount, MITMTrackLength);
        }else if(strcmp(Command,APP_COMMAND_STRING_DISABLE_MEASUREMENT)==0)
        {
            if(Clients[ClientCount].DenyInput==0)
            {
                sprintf(Clients[ClientCount].Response,"Disabling Measurement\n");
                Clients[ClientCount].DenyInput=1;
            }else {
                sprintf(Clients[ClientCount].Response,"Enabling Measurement\n");
                Clients[ClientCount].DenyInput=0;
            }
        }else if(strcmp(Command,APP_COMMAND_STRING_STORE_BATTERY)==0)
        {
            if (i <= strlen(Clients[ClientCount].Command)) {
                sscanf(Clients[ClientCount].Command + i, "%s", Parameters[0]);
                sscanf(Parameters[0],"%f",Clients[ClientCount].Battery_Percentage);
                i += strlen(Parameters[0]) + 1;
            }
        }else if(strcmp(Command,APP_COMMAND_STRING_STORE_TEMPERATURE)==0)
        {
            if (i <= strlen(Clients[ClientCount].Command)) {
                sscanf(Clients[ClientCount].Command + i, "%s", Parameters[0]);
                sscanf(Parameters[0],"%f",Clients[ClientCount].Temperature);
                i += strlen(Parameters[0]) + 1;
            }
        }else if(strcmp(Command,APP_COMMAND_STRING_STORE_PROXIMITY)==0)
        {
            if (i <= strlen(Clients[ClientCount].Command)) {
                sscanf(Clients[ClientCount].Command + i, "%s", Parameters[0]);
                sscanf(Parameters[0],"%d",Clients[ClientCount].Proximity);
                i += strlen(Parameters[0]) + 1;
            }
        }else if(strcmp(Command,APP_COMMAND_STRING_STORE_STATUS)==0)
        {
            if (i <= strlen(Clients[ClientCount].Command)) {
                sscanf(Clients[ClientCount].Command + i, "%s", Parameters[0]);
                sscanf(Parameters[0],"%d",Clients[ClientCount].Status);
                i += strlen(Parameters[0]) + 1;
            }
        }
        Clients[ClientCount].Response+=strlen(Clients[ClientCount].Response);
    }
    Clients[ClientCount].Response=Clients[ClientCount].ResponseBuffer;
}

