/*******************************************************
*   Mahjongg©
*
*   A classic old game.
*
*   @author  TheAbstractCompany, YNOP(ynop@acm.org) 
*   @vertion beta
*   @date    Feb 17 2000
*******************************************************/
#include "Mahjongg.h"

// Application's signature
const char *APP_SIGNATURE = "application/x-vnd.Abstract-Mahjongg";

/*******************************************************
*   Main.. What else do you want from me.
*******************************************************/
int main(int argc, char* argv[]){
   Mahjongg *app = new Mahjongg();
   
   app->Run();
   delete app;
   return B_OK;
}
