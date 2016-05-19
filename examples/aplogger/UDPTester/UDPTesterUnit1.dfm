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
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Memo1: TMemo
    Left = 0
    Top = 27
    Width = 524
    Height = 202
    Align = alClient
    Lines.Strings = (
      'Waiting for packets'
      '')
    ScrollBars = ssVertical
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
    Height = 27
    Align = alTop
    BevelInner = bvLowered
    BevelOuter = bvNone
    TabOrder = 2
    object Label1: TLabel
      Left = 4
      Top = 6
      Width = 13
      Height = 13
      Caption = 'IP:'
    end
    object Label2: TLabel
      Left = 330
      Top = 6
      Width = 22
      Height = 13
      Caption = 'Port:'
    end
    object SendButton1: TButton
      Left = 390
      Top = 3
      Width = 41
      Height = 21
      Caption = 'Send'
      TabOrder = 0
      OnClick = SendButton1Click
    end
    object ToAddressEdit1: TEdit
      Left = 260
      Top = 3
      Width = 67
      Height = 21
      TabOrder = 1
      Text = '192.168.1.9'
    end
    object PortEdit1: TEdit
      Left = 355
      Top = 3
      Width = 33
      Height = 21
      Color = clWhite
      TabOrder = 2
      Text = '1025'
    end
    object SendMsgEdit1: TEdit
      Left = 435
      Top = 3
      Width = 85
      Height = 21
      TabOrder = 3
      Text = '>QueryAPs'
    end
    object URLComboBox1: TComboBox
      Left = 19
      Top = 3
      Width = 230
      Height = 21
      ItemHeight = 13
      TabOrder = 4
      Text = '192.168.1.9'
      OnClick = URLComboBox1Click
    end
  end
  object Timer1: TTimer
    Enabled = False
    Left = 8
    Top = 56
  end
  object DnsQuery1: TDnsQuery
    Port = '53'
    Addr = '192.168.1.1'
    Left = 48
    Top = 56
  end
end
