#include<stdio.h>

int main(int argc,char *argv[])
{
    FILE *Input,*OutputTime,*OutputDisp;
    long time[2],distance[2],displacement,NOV,currentcount=0,currenttime=0,previousdistance=0,currentdistance;
    Input=fopen("input.txt","r");
    OutputTime=fopen("outputtime.txt","w");
    OutputDisp=fopen("outputdisp.txt","w");
    fscanf(Input,"%lu\n",&NOV);
    fscanf(Input,"%lu,%lu\n",&(time[1]),&(distance[1]));
    currentcount++;
    printf("loaded values %lu,%lu\n",time[1],distance[1]);
    
    for(;currentcount<NOV;currentcount++)
    {
        time[0]=time[1];
        distance[0]=distance[1];
        fscanf(Input,"%lu,%lu\n",&(time[1]),&(distance[1]));
        //currentcount++;
        //printf("loaded values %d,%d\n",time[1],distance[1]);
        while(currenttime<time[1])
        {
            currentdistance=(currenttime-time[0])*(distance[1]-distance[0])/(time[1]-time[0]);
            currentdistance=currentdistance+distance[0];
            displacement=currentdistance-previousdistance;
            //printf("Calculated from currenttime=%d, values currentdistance= %d\n",currenttime,currentdistance);
            previousdistance=currentdistance;
            fprintf(OutputTime,"%lu\n",currenttime);
            fprintf(OutputDisp,"%lu\n",displacement);
            currenttime+=4000;
            if((currenttime%1000000)==0)
            {
                printf("currentcount=%lu, currentime %lu,currentdistance=%lu\n",currentcount,currenttime,currentdistance);
            }
        }
    }
    fclose(Input);
    fclose(OutputTime);
    fclose(OutputDisp);
}
