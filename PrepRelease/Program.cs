using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;

namespace PrepRelease
{
    internal class Program
    {
        static int Main(string[] args)
        {
            // Parse the desired version
            if (args == null || args.Length != 1 || !Version.TryParse(args[0], out Version version))
            {
                Console.WriteLine("Expected exactly one argument in version format, e.g. \"1.2.34.5\".");
                return 1;
            }

            // Find the root of the project
            string root = Path.GetDirectoryName(Path.GetDirectoryName(Path.GetDirectoryName(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location))));
            Console.WriteLine($"Detected project root to be \"{root}\"");

            // Update the version in AssemblyInfo.cs
            ReplaceVersionInFiles(root, "AssemblyInfo.cs", "AssemblyVersion\\(\"[0-9]+?\\.[0-9]+?\\.[0-9]+?\\.[0-9]+?\"\\)", $"AssemblyVersion(\"{version.ToString()}\")");
            ReplaceVersionInFiles(root, "AssemblyInfo.cs", "AssemblyFileVersion\\(\"[0-9]+?\\.[0-9]+?\\.[0-9]+?\\.[0-9]+?\"\\)", $"AssemblyFileVersion(\"{version.ToString()}\")");

            // Update the version in AFE2SaveEditor.rc
            ReplaceVersionInFiles(root, "AFE2SaveEditor.rc", "FILEVERSION [0-9]+,[0-9]+,[0-9]+,[0-9]+", $"FILEVERSION {version.ToString().Replace('.', ',')}");
            ReplaceVersionInFiles(root, "AFE2SaveEditor.rc", "PRODUCTVERSION [0-9]+,[0-9]+,[0-9]+,[0-9]+", $"PRODUCTVERSION {version.ToString().Replace('.', ',')}");
            ReplaceVersionInFiles(root, "AFE2SaveEditor.rc", "VALUE \"FileVersion\", \"[0-9]+.[0-9]+.[0-9]+.[0-9]+\"", $"VALUE \"FileVersion\", \"{version.ToString()}\"");
            ReplaceVersionInFiles(root, "AFE2SaveEditor.rc", "VALUE \"ProductVersion\", \"[0-9]+.[0-9]+.[0-9]+.[0-9]+\"", $"VALUE \"ProductVersion\", \"{version.ToString()}\"");

            // Update the version in Version.hpp
            ReplaceVersionInFiles(root, "Version.hpp", "AFE2SAVEEDITOR_VERSION \"[0-9]+.[0-9]+.[0-9]+.[0-9]+\"", $"AFE2SAVEEDITOR_VERSION \"{version.ToString()}\"");

            return 0;
        }

        private static void ReplaceVersionInFiles(string root, string fileNameMask, string regex, string version)
        {
            string[] files = Directory.GetFiles(root, fileNameMask, SearchOption.AllDirectories);
            foreach (string filePath in files)
            {
                string content = File.ReadAllText(filePath);
                content = Regex.Replace(content, regex, version);
                File.WriteAllText(filePath, content);
                Console.WriteLine($"Patched version to {version} in \"{filePath}\"");
            }
        }
    }
}
