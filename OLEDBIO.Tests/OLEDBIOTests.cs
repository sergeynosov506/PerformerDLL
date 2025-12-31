using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Extensions.Configuration;
using PerformerDLL.Interop.Wrappers;
using PerformerDLL.Interop.Common;
using System.IO;

namespace OLEDBIO.Tests;

[TestClass]
public class OLEDBIOTests
{
    private IConfiguration _configuration;
    private TestSettings _settings;

    [TestInitialize]
    public void Setup()
    {
        var builder = new ConfigurationBuilder()
            .SetBasePath(Directory.GetCurrentDirectory())
            .AddJsonFile("appsettings.json", optional: false, reloadOnChange: true);
        
        _configuration = builder.Build();
        _settings = _configuration.GetSection("TestSettings").Get<TestSettings>();
    }

    [TestCleanup]
    public void Cleanup()
    {
        // Try to free resources after each test
        // OLEDBIO.FreeOLEDBIO(); 
    }

    [TestMethod]
    public void InitializeOLEDBIO_WithInvalidAlias_ShouldFail()
    {
        // Arrange
        string invalidAlias = "INVALID_DB_ALIAS_12345";
        NativeERRSTRUCT err = new NativeERRSTRUCT();

        // Act
        // InitializeOLEDBIO(alias, mode, type, date, prepareFlags, ref err)
        PerformerDLL.Interop.Wrappers.OLEDBIO.InitializeOLEDBIO(
            invalidAlias, 
            "", 
            "", 
            20240101, 
            0, 
            ref err);

        // Assert
        Console.WriteLine($"Init Result: Success={err.IsSuccess}, SqlError={err.iSqlError}, ID={err.iID}");
        
        Assert.IsFalse(err.IsSuccess, "InitializeOLEDBIO should fail with invalid alias");
        Assert.AreEqual(-1, err.iSqlError, "iSqlError should be -1 for connection failure");
    }

    [TestMethod]
    public void InitializeOLEDBIO_WithValidAlias_ShouldSucceed()
    {
        // Arrange
        string validAlias = _settings.Database; // Should be "INV_Demo_sales"
        NativeERRSTRUCT err = new NativeERRSTRUCT();

        Console.WriteLine($"Testing with valid alias: {validAlias}");

        // Act
        PerformerDLL.Interop.Wrappers.OLEDBIO.InitializeOLEDBIO(
            validAlias, 
            "", 
            "", 
            20240101, 
            0, 
            ref err);

        // Assert
        Console.WriteLine($"Init Result: Success={err.IsSuccess}, SqlError={err.iSqlError}, ID={err.iID}");
        
        Assert.IsTrue(err.IsSuccess, $"InitializeOLEDBIO should succeed with valid alias '{validAlias}'. Error: {err.iSqlError}");
    }
}

public class TestSettings
{
    public string Server { get; set; }
    public string Database { get; set; }
    public string LogFile { get; set; }
    public int AsOfDate { get; set; }
}
