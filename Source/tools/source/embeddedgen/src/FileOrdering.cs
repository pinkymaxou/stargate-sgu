using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace embeddedgen
{
    class FileOrdering : IComparer<FileInfo>
    {
        public enum EFileTypePriority
        {
            html,
            css,
            js,
            json,
            txt,
            ico,
            other
        }

        static EFileTypePriority GetTypePriority(string filename)
        {
            switch(System.IO.Path.GetExtension(filename).ToLower())
            {
                case ".html": return EFileTypePriority.html;
                case ".css": return EFileTypePriority.css;
                case ".js": return EFileTypePriority.js;
                case ".json": return EFileTypePriority.json;
                case ".ico": return EFileTypePriority.ico;
                case ".txt": return EFileTypePriority.txt;
            }
            return EFileTypePriority.other;
        }

        /// <summary>
        /// Order by file type.
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns></returns>
        public int Compare(FileInfo x, FileInfo y)
        {
            EFileTypePriority xP = GetTypePriority(x.Name);
            EFileTypePriority yP = GetTypePriority(y.Name);
            if (xP < yP)
                return -1;
            if (xP > yP)
                return 1;

            //if (x.Extension == y.Extension)
            //{
            //    if (x.Length < y.Length)
            //        return -1;
            //    if (x.Length > y.Length)
            //        return 1;
            //    return 0;
            //}

            return x.Name.CompareTo(y.Name);
        }
    }
}
