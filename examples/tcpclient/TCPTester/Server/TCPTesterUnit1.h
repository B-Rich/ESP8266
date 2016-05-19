//---------------------------------------------------------------------------
#ifndef TCPTesterUnit1H
#define TCPTesterUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Sockets.hpp>
#include <ComCtrls.hpp>
#include <ScktComp.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TMemo *Memo1;
    TStatusBar *StatusBar1;
    TServerSocket *ServerSocket1;
    TPanel *Panel1;
    TButton *OpenButton1;
    TButton *CloseButton1;
    TEdit *PortEdit1;
    TLabel *Label1;
	void __fastcall OpenButton1Click(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall ServerSocket1Accept(TObject *Sender,TCustomWinSocket *Socket);
    void __fastcall ServerSocket1ClientRead(TObject *Sender,TCustomWinSocket *Socket);
    void __fastcall ServerSocket1ClientConnect(TObject *Sender,TCustomWinSocket *Socket);
    void __fastcall ServerSocket1ClientDisconnect(TObject *Sender,
          TCustomWinSocket *Socket);
    void __fastcall CloseButton1Click(TObject *Sender);

private:	// User declarations

public:		// User declarations

	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
