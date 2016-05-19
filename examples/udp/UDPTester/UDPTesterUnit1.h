//---------------------------------------------------------------------------
#ifndef UDPTesterUnit1H
#define UDPTesterUnit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
#define WM_SOCKETREADY  WM_USER + 2000
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TMemo *Memo1;
    TTimer *Timer1;
    TStatusBar *StatusBar1;
    TPanel *Panel1;
    TButton *SendButton1;
    TEdit *ToAddressEdit1;
    TLabel *Label1;
    TEdit *PortEdit1;
    TLabel *Label2;
    TEdit *SendMsgEdit1;
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall SendButton1Click(TObject *Sender);

private:	// User declarations
    WSADATA wsda;
    SOCKET hSock;
    SOCKADDR_IN addr, remote_addr;
    SOCKADDR_IN ToAddress;

    bool ErrorOccurred;

public:		// User declarations

    void __fastcall WMSocketMsg(TMessage &Msg);
    BEGIN_MESSAGE_MAP
        MESSAGE_HANDLER(WM_SOCKETREADY, TMessage, WMSocketMsg)
    END_MESSAGE_MAP(TForm) ;

    __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
