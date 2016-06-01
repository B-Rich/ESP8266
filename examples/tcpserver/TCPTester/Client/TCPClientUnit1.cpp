//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "TCPClientUnit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner) : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button1Click(TObject *Sender)
{
    ClientSocket1->Host    = HostEdit1->Text;
    ClientSocket1->Address = MyIPEdit1->Text;
    ClientSocket1->Port    = PortEdit1->Text.ToInt();
    ClientSocket1->Active  = true;
    StatusBar1->SimpleText = "Talking on Port: " + AnsiString(ClientSocket1->Port);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CloseButton1Click(TObject *Sender)
{
    ClientSocket1->Active = false;
    StatusBar1->SimpleText = "Connection Closed";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
    ClientSocket1->Active = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SendButton1Click(TObject *Sender)
{
    ClientSocket1->Socket->SendText(Edit1->Text);
}
//---------------------------------------------------------------------------

