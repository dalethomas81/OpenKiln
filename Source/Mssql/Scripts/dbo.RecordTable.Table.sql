USE [automation_historical]
GO
/****** Object:  Table [dbo].[RecordTable]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
CREATE TABLE [dbo].[RecordTable](
	[TransactionDateTime] [datetime] NULL,
	[RecordNumber] [bigint] IDENTITY(1,1) NOT NULL,
	[RecordName] [varchar](255) NULL,
 CONSTRAINT [PK_RecordTable] PRIMARY KEY CLUSTERED 
(
	[RecordNumber] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO
ALTER TABLE [dbo].[RecordTable] ADD  CONSTRAINT [DF_RecordTable_TransactionDateTime]  DEFAULT (getutcdate()) FOR [TransactionDateTime]
GO
