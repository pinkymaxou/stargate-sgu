using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace embeddedgen
{
    class ConfigReader
    {
        public class FileConfig
        {
            public string RelativeFilename { get; internal set; } = "";
        }

        /// <summary>
        /// Get file configs
        /// </summary>
        /// <param name="fullname"></param>
        /// <returns></returns>
        public static IEnumerable<FileConfig> GetConfig(string fullname)
        {
            // Read the file and display it line by line.  
            foreach (string line in System.IO.File.ReadLines(fullname))
            {
                if (line.Trim().Length == 0)
                    continue;

                yield return new FileConfig()
                {
                    RelativeFilename = line,
                };
            }
        }
    }
}
