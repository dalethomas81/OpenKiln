USE [automation_historical]
GO
/****** Object:  StoredProcedure [dbo].[DeactivateTag]    Script Date: 7/18/2021 12:20:45 PM ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
-- =============================================
-- Author:		Dale Thomas
-- Create date: 08/05/2018
-- Description:	deactivates tags in the tag table
-- =============================================
CREATE PROCEDURE [dbo].[DeactivateTag] 
	-- Add the parameters for the stored procedure here
	@TAGNAME nVarchar(255) = 0
AS
BEGIN
	-- SET NOCOUNT ON added to prevent extra result sets from
	-- interfering with SELECT statements.
	SET NOCOUNT ON;

    -- Insert statements for procedure here
	DECLARE @TagIndex smallint

	SELECT @TagIndex = TagIndex
	FROM TagTable
	WHERE TagName = @TAGNAME

	UPDATE TagTable
	SET [Status] = 'InActive'
	WHERE TagIndex = @TagIndex

END

GO
