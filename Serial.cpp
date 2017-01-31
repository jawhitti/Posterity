/*
This program is the simplest of simple serial communications apps.
I wrote it to learn myself how to use a modem in windows.  I have had
success using two copies of it to communicate across phone lines with
one another.  For other serial devices, I doubt this program is Appropriate.
 It is written under TC++ 3.0/win and should work under any Borland
windows compiler in windows 3.1.  Be sure to include the resource file
in the program, or create your own resources.

	An interesting note on functionality: Most of the demo programs
I've seen advocate using the Idle() procedure in the TApplication object
to process incoming information from the port.  However, from my own
experience, THIS IS NOT THE WAY TO DO IT!  Instead, this program lets
Windows send it messages telling it when to read the buffer etc...

Good luck, and email me at

jawhitti@midway.ecn.uoknor.edu

if you have any questions.  The program runs to near 600 lines, but is
really quite simple.  Don't be intimidated!
*/


#define STRICT
#define WIN31

/* if you want to use the dynamic libraries, set memory model to large,
and case-sensitive link to off, and leave _CLASSDLL defined.  Otherwise
comment it out. */

#define _CLASSDLL

#include <owl.h>
#include <edit.h>
/* bwindow is not necessary, it just makes a gray window. The program
works just the same if you derive TSerialWindow from TWindow. */
#include <bwindow.h>

// #defines for the program. Change them to fit your resources,etc...
#define CM_DIAL 101
#define CM_HANGUP 102
#define UM_ERROR WM_USER+1
#define UM_DATA WM_USER+2
// Size of buffer to recieve incoming text...
#define MAXINPUTLENGTH 80
// Size of Phone number buffer...
#define PHONEINPUTLENGTH 25


_CLASSDEF(TSerialApp)
// TSerial app is just a TApplication that sets up a TSerialWindow
class TSerialApp : public TApplication
{
  public:
  TSerialApp(LPSTR Aname, HINSTANCE hInstance, HINSTANCE hPrevInstance,
	 LPSTR lpCmdLine, int nCmdShow)
	 : TApplication(Aname, hInstance, hPrevInstance, lpCmdLine, nCmdShow){};
  virtual void InitMainWindow();
};

_CLASSDEF(TSerialWindow)
class TSerialWindow: public TBWindow
{
/* This class is the backbone of the program.  It likely is not the BEST
way to do things, but it works.  I plan to modify it to make it more
easily inherited.  Do as you please... */

	public:
// Handle to Borland custom controls...
		HANDLE BWCCMod;

		char PhoneNumber[PHONEINPUTLENGTH];
		char Command[MAXINPUTLENGTH];
//CPort holds the current open COM Port, y controls a primitive
//Screen scroll.
		int  CPort,y;
		TSerialWindow(PTWindowsObject AParent, LPSTR ATitle);
		virtual void SetupWindow();
		virtual void CloseWindow();
        virtual void CommEvent();
		virtual void CheckReceiveBuffer();
		virtual void DialPhone(RTMessage Msg)  = [CM_FIRST+ 101];
		virtual void SendText(RTMessage Msg)   = [CM_FIRST+ 102];
        virtual void GetStatus(RTMessage Msg)  = [CM_FIRST+ 103];
		virtual void SendCommand(RTMessage Msg)= [CM_FIRST+ 104];
		virtual void SerError(RTMessage Msg)  = [WM_FIRST+UM_ERROR];
		virtual void SerData(RTMessage Msg)   = [WM_FIRST+UM_DATA];

//This procedure is important, and must be enabled in SetupWindow
//with the ENABLECOMMNOTIFICATION call.
		virtual int CommNotification(RTMessage Msg) = [WM_COMMNOTIFY];
 };

//These dialog classes control some simple dialogs.  Play with them
//to make the program more fun.

//Get user input.
_CLASSDEF(TMyInputDialog)
class TMyInputDialog : public TDialog
	{
    public : TMyInputDialog(PTWindowsObject AParent, LPSTR AResource);
	};

//Get Phone number
_CLASSDEF(TPhoneInputDialog)
class TPhoneInputDialog : public TDialog
	{
    public : TPhoneInputDialog(PTWindowsObject AParent, LPSTR AResource);
	};

//Select a COM port.  Default is COM2
_CLASSDEF(TCommPortDialog)
class TCommPortDialog : public TDialog
	{
	public : TCommPortDialog(PTWindowsObject AParent, LPSTR AResource);
	private:
	void PickOne(RTMessage Msg) =  [ID_FIRST + 101];
    void PickTwo(RTMessage Msg) =  [ID_FIRST + 102];
    void PickThree(RTMessage Msg)= [ID_FIRST + 103];
	void PickFour(RTMessage Msg) = [ID_FIRST + 104];

	};


void TSerialApp::InitMainWindow()
	{
	MainWindow = new TSerialWindow(NULL,Name);
	}

//******************************************************
// TSerialWindow member functions...                   *
//******************************************************

TSerialWindow::TSerialWindow(PTWindowsObject AParent, LPSTR ATitle)
	: TBWindow(AParent,ATitle)
//Simple constructor does nothing special
	{
	PhoneNumber[0] = '\0';
	CPort = -1; y=10;
	AssignMenu("MENU_1");
    BWCCMod=LoadLibrary("BWCC.DLL");
    }

void TSerialWindow::SetupWindow()
	{
    int WhichPort;
	DCB CommData;
	LPSTR ACommPort;

	TBWindow::SetupWindow();
	MessageBeep(MB_OK);
    //Get the COM Port preference...
	WhichPort=GetModule()->ExecDialog(new TCommPortDialog(this, "DIALOG_3"));
	switch(WhichPort)
		{
		case 1: CPort = OpenComm("COM1", 1024, 1024); break;
		case 3: CPort = OpenComm("COM3", 1024, 1024); break;
		case 4: CPort = OpenComm("COM4", 1024, 1024); break;
		default:CPort = OpenComm("COM2", 1024, 1024); break;
		}
   //OpenComm is <0 if the port fails to open...
		if(CPort < 0)
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(HWindow,"Comm port not open","Uh-Oh",MB_OK);
		}
    //Retrieve default Port settings...
	GetCommState(CPort, &CommData);
    //Adjust for app specific parameters....
	CommData.BaudRate = 2400;
	CommData.ByteSize = 8;
	CommData.Parity = NOPARITY;
	CommData.StopBits = ONESTOPBIT;
	CommData.DsrTimeout =1000;
    //Set the COM port to new parameters...
    if(SetCommState(&CommData)!=0)
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(HWindow,"Comm Port not configured","Error",MB_OK+MB_ICONEXCLAMATION);
		}
     //Flush Input and output buffers...
	 FlushComm(CPort,0); FlushComm(CPort,1);

     //Tell windows to send CommNotification messages to this window...
	 if (EnableCommNotification(CPort,HWindow,160,-1)==0)
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(HWindow,"EnableCommNotification Failed","Error",MB_OK+MB_ICONEXCLAMATION);
		}
     //Tell windows to notify program of ANY comport message...
	 SetCommEventMask(CPort,0xFFFF);

     //Modem Initialization string.  Set to whatever...
	 WriteComm(CPort,"ATE1 S12=10\r",13);
 	}


void TSerialWindow::CloseWindow()
	{
		CloseComm(CPort);
		TBWindow::CloseWindow();
	}

void TSerialWindow::CheckReceiveBuffer()

//This program looks in the recieve buffer and retrieves information
//in it into Buffer.  If an error occurs, a UM_ERROR message is sent,
//else a UM_DATA message is sent, with a pointer to the data in LParam.
	{
	int Ret,Err,i;
	char Buffer[80];
	COMSTAT Comstat;

	for(i=0;i<80;i++)Buffer[i]='\0';
	if(CPort>=0)
		{
		Ret = ReadComm(CPort, Buffer, 80);
		if(Ret<0)
			{
			Err = GetCommError(CPort, &Comstat);
			if(Err>0)SendMessage(HWindow, UM_ERROR, 3, Err);
			}
		if(Ret>0)
        	{
			SendMessage(HWindow, UM_DATA,Ret,(long)Buffer);
			}
		 }
	}

void TSerialWindow::CommEvent()
	{
	int event;
	char Buffer[80];

	//Simple procedure Filters event messages.  For most, simply
	//sends a message to itself to write on the sreen what the
	//message was.  Events are coded it a bitfield, so each IF checks
	//for a specific flag.  Several flags may be checked at once, so
    //a SWITCH is not practical.
    event = GetCommEventMask(CPort,0xFFFF);
	if(event & EV_BREAK)
		{
		wsprintf(Buffer,"EV_BREAK");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_CTS)
		{
		wsprintf(Buffer,"EV_CTS");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_CTSS)
		{
		wsprintf(Buffer,"EV CTSS...");
		PostMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_DSR)
		{
		wsprintf(Buffer,"EV_DSR");
		SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_ERR)
		{
		wsprintf(Buffer,"EV_ERR");
		SendMessage(HWindow,UM_ERROR,GetCommError(CPort,NULL),2);
		}
	if(event & EV_PERR)
		{
		wsprintf(Buffer,"EV_PERR");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}   
 	if(event & EV_RING)
		{
        wsprintf(Buffer,"The telephone is ringing.");
		PostMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_RLSD)
		{
		wsprintf(Buffer,"EV_RLSD");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
  	if(event & EV_RLSDS)
		{
		wsprintf(Buffer,"EV_RLSDS");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	if(event & EV_RXCHAR)
		{
		wsprintf(Buffer,"EV_RXCHAR");
		SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}  
	if(event & EV_BREAK)
		{
		wsprintf(Buffer,"EV_BREAK");
		SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
        }
	if(event & EV_TXEMPTY)
		{
		wsprintf(Buffer,"EV_TXEMPTY");
        SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}     
	}

int TSerialWindow::CommNotification(RTMessage Msg)
// This procedure is invoked when Windows sends a CommNotification
// message.  There are 3 types of messages, this program only really
// repsonds to CN_RECEIVE messages by reading the input buffer, and to
// CN_EVENT messages.  CN_EVENT messages are further analyzed by
// TSerialWindow::CommEvent.

	{
	char Buffer[80];
    int message;

	message=Msg.LP.Lo;
	if(message & CN_EVENT)
		{
		CommEvent();
		}
	if(message & CN_RECEIVE)
		{
	 /*	wsprintf(Buffer,"CN RECEIVE");
		SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer); */
		CheckReceiveBuffer();
		FlushComm(CPort,1);
		}
	if(message & CN_TRANSMIT)
		{
		wsprintf(Buffer,"CN TRANSMIT");
		SendMessage(HWindow,UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
    return 0;
	}

void TSerialWindow::DialPhone(RTMessage Msg)
	{
	//Simple procedure to dial the Phone.  Note that a '\r' (carrage return)
	//must be sent to the port for the modem to dial properly. 
	wsprintf(PhoneNumber,"");
	if(GetModule()->ExecDialog(new TPhoneInputDialog(this, "DIALOG_1"))!=IDCANCEL)
    	{
		if(lstrlen(PhoneNumber)>0)
			{
			WriteComm(CPort,"ATDT ",5);
            WriteComm(CPort,PhoneNumber,lstrlen(PhoneNumber));
			WriteComm(CPort,"\r",1);
			}
		}

	}

void TSerialWindow::SendText(RTMessage Msg) 
	{
//This is really an arcane way to communicate, but this procedure
//pops up a dialog in response to a menu command and inputs text to send.
//It then sends the text, and Posts a message to echo it to the window.
//You probably will want to overhaul it.
    wsprintf(Command,"");
	if(GetModule()->ExecDialog(new TMyInputDialog(this, "DIALOG_2"))!=IDCANCEL)
		{
		if(lstrlen(Command)>0)
			{
			WriteComm(CPort,Command,lstrlen(Command));
			PostMessage(HWindow,UM_DATA,lstrlen(Command),(long)Command);
			}
		}
	}


void TSerialWindow::SendCommand(RTMessage Msg)
	{
//This silly procedure doesn't even work(That is, it wont send the
//modem into command mode) but I'm to lazy to fix it.  What is needed is
//to send +++ to the serial port, and then WAIT for the modem to respond
//with OK.  THEN send the command.  To hang up from the program as it
//is written now, use the SEND dialog to send +++(no carriage return!)
//and wait for OK.  Then use SEND again to send AT H to hang up, or
//whatever it is you want to do.

	wsprintf(Command,"AT H");
	WriteComm(CPort,"+++ ",4);
	MessageBeep(0);
	if(GetModule()->ExecDialog(new TMyInputDialog(this, "DIALOG_2"))!=IDCANCEL)
    	{
		if(lstrlen(Command)>0)
			{
			WriteComm(CPort,Command,lstrlen(Command));
			wsprintf(Command,"\r");
			WriteComm(CPort,Command,1);
			}
		}
	}


void TSerialWindow::GetStatus(RTMessage)

//This procedure gets the status of the port into a COMSTAT
//structure, and alerts you for any set flags.  The flags are
//all explained in help.
	{
	int Err;
	char Buffer[80];
	COMSTAT Comstat;

    wsprintf(Buffer,"\0");
	Err=GetCommError(CPort,&Comstat);
    if(Err>0) SendMessage(HWindow,UM_ERROR,Err,1);
	if(Comstat.status)
		{
		if(Comstat.status & CSTF_CTSHOLD)
			{
			wsprintf(Buffer,"CSTF_CTSHOLD");
		    SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_DSRHOLD)
			{
			wsprintf(Buffer,"CSTF_DSRHOLD");
			SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_RLSDHOLD)
			{
			wsprintf(Buffer,"CSTF_RLSHOLD");
			SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_XOFFHOLD)
			{
			wsprintf(Buffer,"CSTF_XOFFHOLD");
			SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_XOFFSENT)
			{
			wsprintf(Buffer,"CSTF_XOFFSENT\0");
			SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_EOF)
			{
			wsprintf(Buffer,"CSTF_EOF\0");
			SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
		if(Comstat.status & CSTF_TXIM)
			{
			wsprintf(Buffer,"CSTF_TXIM\0");
		    SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
			}
        }
	else //No flags set in COMSTAT
   		{
		wsprintf(Buffer,"Ok");
   		SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
		}
	 wsprintf(Buffer,"In queue contains %i characters.",Comstat.cbInQue);
     SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
	 wsprintf(Buffer,"Out queue contains %i characters.",Comstat.cbOutQue);
     SendMessage(HWindow, UM_DATA,lstrlen(Buffer),(long)Buffer);
    }


void TSerialWindow::SerError(RTMessage Msg)
//This is a simple procedure to write error messages on the window.
//Curiously, sometimes ReadComm() will return an error state -1, but
//GetCommErr() will return 0, stating no error occured.  Ignoring these
//seems to work fine.
	{
	HDC hDC;
	char astr[5];
    int Err;

	hDC=GetDC(HWindow);
	Err=Msg.LParam;
	wsprintf(astr,"%i  %i",Msg.LParam,Msg.WParam);
	TextOut(hDC,10,y,"Error:",6);
	TextOut(hDC,60,y,astr,lstrlen(astr));
    //primitive scroll...
	y+=15;
    if(y>300) y=10;
	ReleaseDC(HWindow,hDC);
	}


void TSerialWindow::SerData(RTMessage Msg)
	//This procedure Outputs all data received from the COM port.
    //The bulk of it simply edits out '\r' and '\n' characters.
	{
	HDC hDC;
    char Instring[80],Outstring[80];
	int i,j;

	for(i=0;i<80;i++)
		{
		Instring[i]='\0';
		Outstring[i]='\0';
        }
    wsprintf(Outstring,"");
	wsprintf(Instring,(LPSTR)Msg.LParam);
	if(lstrlen(Instring)>0)
    	{
		for(i=0,j=0;Instring[i]!='\0';i++)
			{
			if((Instring[i]!='\r')&&(Instring[i]!='\n'))
				{
				Outstring[j]=Instring[i];
				j++;
				}
			else
				{
				if(Instring[i]!='\r')y+=15;
				}
			}
	 	hDC=GetDC(HWindow);   
		TextOut(HWindow,(LPSTR)Outstring,j);
		y+=15;
		if(y>Attr.H) y=10;
	 	ReleaseDC(HWindow,hDC);  
		}
	}
/*******************************
 **    TMyInputDialog Functions*
 *******************************/


TMyInputDialog::TMyInputDialog(PTWindowsObject AParent, LPSTR AResource)
				 :TDialog(AParent,AResource)
		{
		new TEdit(this,101,MAXINPUTLENGTH);
		TransferBuffer = (void far *) &((TSerialWindow*)Parent)->Command;
        }



TPhoneInputDialog::TPhoneInputDialog(PTWindowsObject AParent, LPSTR AResource)
				 :TDialog(AParent,AResource)
		{
		new TEdit(this,101,PHONEINPUTLENGTH);
		TransferBuffer = (void far *) &((TSerialWindow*)Parent)->PhoneNumber;
		}

TCommPortDialog::TCommPortDialog(PTWindowsObject AParent, LPSTR AResource)
				 :TDialog(AParent,AResource)
		{
		}

void TCommPortDialog::PickOne(RTMessage Msg)
	{
	ShutDownWindow(1);
	}

void TCommPortDialog::PickTwo(RTMessage Msg)
	{
	ShutDownWindow(2);
	}

void TCommPortDialog::PickThree(RTMessage Msg)
	{
	ShutDownWindow(3);
	}

void TCommPortDialog::PickFour(RTMessage Msg)
	{
	ShutDownWindow(4);
    }

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	 LPSTR lpCmdLine, int nCmdShow)
       {
	   TSerialApp App("Modem",hInstance,hPrevInstance,lpCmdLine,nCmdShow);
	   App.Run();
       return App.Status;
       }

