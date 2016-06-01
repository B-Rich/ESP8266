//---------------------------------------------------------------------------
#ifndef TCPClientUnit1H
#define TCPClientUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ScktComp.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TEdit *Edit1;
    TStatusBar *StatusBar1;
    TClientSocket *ClientSocket1;
    TButton *SendButton1;
    TButton *Button1;
    TButton *CloseButton1;
    TEdit *HostEdit1;
    TLabel *Label1;
    TEdit *MyIPEdit1;
    TLabel *Label2;
    TEdit *PortEdit1;
    TLabel *Label3;
    void __fastcall SendButton1Click(TObject *Sender);
    void __fastcall Button1Click(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall CloseButton1Click(TObject *Sender);

private:	// User declarations

public:		// User declarations

    __fastcall TForm1(TComponent* Owner);
    
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
