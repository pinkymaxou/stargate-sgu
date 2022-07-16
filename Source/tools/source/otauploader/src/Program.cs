using System;
using System.IO;
using System.Net;
using CommandLine;

namespace otauploader
{
    class Program
    {
        public class Options
        {
            [Option("input", Required = false, HelpText = "Input filename")]
            public string InputFilename { get; set; } = "";

            [Option("destaddr", Required = false, HelpText = "Destination address (URL)")]
            public string AddressURL { get; set; } = "";
        }

        static void Main(string[] args)
        {
            try
            {
                Parser.Default.ParseArguments<Options>(args)
                    .WithParsed<Options>(async o =>
                    {
                        WebClient myWebClient = new WebClient();

                        Console.Error.WriteLine($"Starting to upload: '{ o.InputFilename }', address: '{ o.AddressURL }'");

                        byte[] buffers = File.ReadAllBytes(o.InputFilename);
                        
                        myWebClient.UploadProgressChanged += MyWebClient_UploadProgressChanged;
                        myWebClient.UploadData(new Uri(o.AddressURL), "POST", buffers);
                        
                        Console.Error.WriteLine("Done!");
                    });
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error: { ex.Message }");
            }
        }

        private static void MyWebClient_UploadProgressChanged(object sender, UploadProgressChangedEventArgs e)
        {
            Console.Error.WriteLine($"Progress: {e.ProgressPercentage} %");
        }
    }
}
