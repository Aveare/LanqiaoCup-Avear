#include <REGX51.H>

void main()
{
    while(1)
    {
      
        if(P1_0 == 0) 
            P1_6 = 1;  
        else         
            P1_6 = 0; 

        if(P1_1 == 0) 
            P1_7 = 1;  
        else          
            P1_7 = 0; 
    }
}