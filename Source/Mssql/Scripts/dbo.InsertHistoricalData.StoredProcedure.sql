USE [automation_historical]
GO
/****** Object:  StoredProcedure [dbo].[InsertHistoricalData]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		Dale Thomas
-- Create date: 2018-06-19
-- Description:	Insert a record into the historical db
-- =============================================
CREATE PROCEDURE [dbo].[InsertHistoricalData]
	-- Add the parameters for the stored procedure here
	@TAGNAME nVarchar(255) = 0,
	@VALUE nVarchar(255) = 0
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- insert statements for procedure here
	DECLARE @RecordNumber bigint
	DECLARE @TransactionDateTime datetime
	DECLARE @TagIndex smallint
	DECLARE @Date datetime
	DECLARE @Milli as smallint
	DECLARE @TagType as smallint
	DECLARE @TagDataType as smallint

	SET @Date = GETUTCDATE()
	SET @Date = dateadd(millisecond, -datepart(millisecond, @Date), @Date)
	SET @Milli = format(getdate(), 'fff')
	
	--create record
	INSERT INTO RecordTable (RecordName) VALUES (@TAGNAME)
	SELECT TOP 1 @RecordNumber = RecordNumber, @TransactionDateTime = TransactionDateTime 
	FROM RecordTable 
	WHERE RecordName = @TAGNAME
	ORDER BY RecordNumber DESC

	--get tag data
	SELECT @TagIndex = TagIndex, @TagType = TagType, @TagDataType = TagDataType
	FROM TagTable 
	WHERE TagName = @TAGNAME AND
	[Status] = 'Active'


	IF @TagType = 2
	BEGIN
	
		IF @TagDataType = 0
		BEGIN
			--insert into float table
			SELECT @TagIndex = TagIndex FROM TagTable WHERE TagName = @TAGNAME
			INSERT INTO FloatTable(RecordNumber,TransactionDateTime,DateAndTime,Millitm,TagIndex,Val)
			VALUES (@RecordNumber, @TransactionDateTime, @Date,@Milli,@TagIndex,@VALUE)
		END

		IF @TagDataType = 1
		BEGIN
			--insert into float table
			SELECT @TagIndex = TagIndex FROM TagTable WHERE TagName = @TAGNAME
			INSERT INTO FloatTable(RecordNumber,TransactionDateTime,DateAndTime,Millitm,TagIndex,Val)
			VALUES (@RecordNumber, @TransactionDateTime, @Date,@Milli,@TagIndex,@VALUE)
		END

	END

	IF @TagType = 3
	BEGIN
	
		IF @TagDataType = 0
		BEGIN
			--insert into float table
			SELECT @TagIndex = TagIndex FROM TagTable WHERE TagName = @TAGNAME
			INSERT INTO FloatTable(RecordNumber,TransactionDateTime,DateAndTime,Millitm,TagIndex,Val)
			VALUES (@RecordNumber, @TransactionDateTime, @Date,@Milli,@TagIndex,@VALUE)
		END
	END

	IF @TagType = 4
	BEGIN
	
		IF @TagDataType = 2
		BEGIN
			--insert into string table
			SELECT @TagIndex = TagIndex FROM TagTable WHERE TagName = @TAGNAME
			INSERT INTO StringTable(RecordNumber,TransactionDateTime,DateAndTime,Millitm,TagIndex,Val)
			VALUES (@RecordNumber, @TransactionDateTime, @Date,@Milli,@TagIndex,@VALUE)
		END
	END

END
GO
