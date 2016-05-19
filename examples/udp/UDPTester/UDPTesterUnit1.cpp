//---------------------------------------------------------------------------
// UDP Tester
//---------------------------------------------------------------------------
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma link "winsock.lib"   // for WINSOCK.DLL
//#pragma link "wssock32.lib"  // for WSSOCK32.DLL
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "UDPTesterUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)   : TForm(Owner)
{
    ErrorOccurred = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
   closesocket(hSock);
   WSACleanup();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
    u_short iPort = PortEdit1->Text.ToIntDef(1025);

    WSAStartup(MAKEWORD(1,1), &wsda);
    hSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(hSock == SOCKET_ERROR) {
        StatusBar1->SimpleText = "Error: Call to socket() failed with:" + AnsiString(WSAGetLastError());
        ErrorOccurred = true;
    }
    else {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(iPort);
        addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(hSock, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {
            StatusBar1->SimpleText = "Error: Call to bind()... failed with: " + AnsiString(WSAGetLastError());
            ErrorOccurred = true;
        }
        if(WSAAsyncSelect(hSock, Handle, WM_SOCKETREADY, FD_READ)) {
            StatusBar1->SimpleText = "Error: Call to WSAAsyncSelect()... failed with: " + AnsiString(WSAGetLastError());
            ErrorOccurred = true;
        }
        StatusBar1->SimpleText = "Waiting for packets...";
    }
}
//----------------------------------------------------------------------------//
void __fastcall TForm1::WMSocketMsg(TMessage &Msg)
{
    if(Msg.Msg != WM_SOCKETREADY) return;

    int i;
    int iRemoteAddrLen;
    char Message[512];

	iRemoteAddrLen = sizeof(remote_addr);
    int ret = recvfrom(hSock, Message, sizeof(Message), 0, (struct sockaddr *)&remote_addr, &iRemoteAddrLen);
	if(ret == SOCKET_ERROR) {
        StatusBar1->SimpleText = "Error: Call to recvfrom() failed with: " + AnsiString(WSAGetLastError());
        return;
    }
    AnsiString Tmp;
	Tmp = Tmp.sprintf("Received %d bytes from %s", ret, inet_ntoa(remote_addr.sin_addr));
    Memo1->Lines->Add(Tmp);

    Tmp = "";
    for(i=0; i < ret; i++) Tmp = Tmp + IntToHex(Message[i],2) + " ";
    Tmp = Tmp + "     ";
    for(i=0; i < ret; i++) Tmp = Tmp + AnsiString(Message[i]) + " ";
    Memo1->Lines->Add(Tmp);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SendButton1Click(TObject *Sender)
{
    if(ErrorOccurred) {
        StatusBar1->SimpleText = "Sockets did not open properly";
        return;
    }

    char msg[512];
    strcpy(msg,SendMsgEdit1->Text.c_str());
    int   len = SendMsgEdit1->Text.Length();

    ToAddress.sin_addr.S_un.S_addr = inet_addr(ToAddressEdit1->Text.c_str());
    ToAddress.sin_port = htons(PortEdit1->Text.ToIntDef(1025));
    ToAddress.sin_family = AF_INET;

    int ret = sendto(hSock, msg, len, 0, (struct sockaddr *)&ToAddress, sizeof(ToAddress));
    if(ret == SOCKET_ERROR) {
        StatusBar1->SimpleText = "Error: Call to sendto()... failed with: " + AnsiString(WSAGetLastError());
    }
    else {
        StatusBar1->SimpleText = AnsiString(ret) + " bytes sent";
    }
}
//---------------------------------------------------------------------------


