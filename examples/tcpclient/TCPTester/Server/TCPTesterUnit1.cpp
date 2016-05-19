//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "TCPTesterUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner) : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OpenButton1Click(TObject *Sender)
{
    ServerSocket1->Port    = PortEdit1->Text.ToInt();
	ServerSocket1->Active  = true;
    StatusBar1->SimpleText = "Listening on Port: " + AnsiString(ServerSocket1->Port);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CloseButton1Click(TObject *Sender)
{
	ServerSocket1->Active  = false;
    StatusBar1->SimpleText = "Server Closed";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	ServerSocket1->Active = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ServerSocket1Accept(TObject *Sender, TCustomWinSocket *Socket)
{
    StatusBar1->SimpleText = "Connect to: " + Socket->RemoteAddress;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ServerSocket1ClientRead(TObject *Sender, TCustomWinSocket *Socket)
{
    Memo1->Lines->Add(Socket->ReceiveText());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ServerSocket1ClientConnect(TObject *Sender,TCustomWinSocket *Socket)
{
    Memo1->Lines->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ServerSocket1ClientDisconnect(TObject *Sender,TCustomWinSocket *Socket)
{
    StatusBar1->SimpleText = "Conenction Closed";
}
//---------------------------------------------------------------------------


