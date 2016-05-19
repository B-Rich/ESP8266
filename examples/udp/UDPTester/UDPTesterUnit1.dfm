object Form1: TForm1
  Left = 247
  Top = 121
  Width = 540
  Height = 286
  Caption = 'UDP Tester'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Memo1: TMemo
    Left = 0
    Top = 41
    Width = 524
    Height = 188
    Align = alClient
    Lines.Strings = (
      'Waiting for packets'
      '')
    TabOrder = 0
  end
  object StatusBar1: TStatusBar
    Left = 0
    Top = 229
    Width = 524
    Height = 19
    Panels = <>
    SimplePanel = True
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 524
    Height = 41
    Align = alTop
    BevelInner = bvLowered
    BevelOuter = bvNone
    TabOrder = 2
    object Label1: TLabel
      Left = 7
      Top = 13
      Width = 54
      Height = 13
      Caption = 'IP Address:'
    end
    object Label2: TLabel
      Left = 155
      Top = 13
      Width = 22
      Height = 13
      Caption = 'Port:'
    end
    object SendButton1: TButton
      Left = 224
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Send'
      TabOrder = 0
      OnClick = SendButton1Click
    end
    object ToAddressEdit1: TEdit
      Left = 64
      Top = 10
      Width = 87
      Height = 21
      TabOrder = 1
      Text = '192.168.1.7'
    end
    object PortEdit1: TEdit
      Left = 180
      Top = 10
      Width = 33
      Height = 21
      Color = clWhite
      TabOrder = 2
      Text = '1025'
    end
    object SendMsgEdit1: TEdit
      Left = 304
      Top = 9
      Width = 193
      Height = 21
      TabOrder = 3
      Text = 'Test Message'
    end
  end
  object Timer1: TTimer
    Enabled = False
    Left = 8
    Top = 56
  end
end
