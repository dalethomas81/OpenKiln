USE [automation_historical]
GO
/****** Object:  StoredProcedure [dbo].[GetTagValue]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		Dale Thomas
-- Create date: 08/05/2018
-- Description:	Gets the current value of a tag
-- =============================================
CREATE PROCEDURE [dbo].[GetTagValue] 
	-- Add the parameters for the stored procedure here
	@TAGNAME nVarchar(255) = 0,
	@VALUE nVarchar(255) OUTPUT
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for procedure here
	DECLARE @TagIndex smallint
	DECLARE @TagType as smallint
	DECLARE @TagDataType as smallint
	
	--get tag data
	SELECT @TagIndex = TagIndex, @TagType = TagType, @TagDataType = TagDataType
	FROM TagTable 
	WHERE TagName = @TAGNAME AND
	[Status] = 'Active'

	IF @TagType = 2
	BEGIN
	
		IF @TagDataType = 0
		BEGIN
			--get value from float table
			SELECT @VALUE = val FROM FloatTable WHERE tagindex = @TagIndex
		END

		IF @TagDataType = 1
		BEGIN
			--get value from float table
			SELECT @VALUE = val FROM FloatTable WHERE tagindex = @TagIndex
		END

	END

	IF @TagType = 3
	BEGIN
	
		IF @TagDataType = 0
		BEGIN
			--get value from float table
			SELECT @VALUE = val FROM FloatTable WHERE tagindex = @TagIndex
		END
	END

	IF @TagType = 4
	BEGIN
	
		IF @TagDataType = 2
		BEGIN
			--get value from string table
			SELECT @VALUE = val FROM StringTable WHERE tagindex = @TagIndex
		END
	END

END

GO
