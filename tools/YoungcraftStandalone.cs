// Built by create_standalone.ps1 using the Windows .NET Framework compiler.
// No installer, download, elevation, PowerShell, or external ZIP is used at runtime.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using System.Windows.Forms;

internal static class YoungcraftStandalone
{
    private const string ManifestName = "package-manifest.json";
    private static readonly JavaScriptSerializer Json = new JavaScriptSerializer { MaxJsonLength = 32 * 1024 * 1024 };

    public sealed class FileRecord
    {
        public string path { get; set; }
        public string sha256 { get; set; }
        public long size { get; set; }
    }
    public sealed class Manifest
    {
        public int schema { get; set; }
        public List<FileRecord> files { get; set; }
    }
    public sealed class VersionRecord
    {
        public string payloadSha256 { get; set; }
        public string manifestSha256 { get; set; }
    }
    public sealed class Versions
    {
        public int schema { get; set; }
        public List<VersionRecord> versions { get; set; }
    }

    [STAThread]
    private static int Main(string[] args)
    {
        bool headless = false;
        // Detect verification mode even when another option is malformed.
        foreach (string arg in args)
        {
            if (arg == "--") break;
            if (arg == "--youngcraft-extract-only" || arg == "--youngcraft-verify-only") headless = true;
        }
        try
        {
            if (headless)
            {
                // Explicit UTF-8 writers also work for a GUI subsystem executable
                // without an attached console, and round-trip every Windows path.
                Console.SetOut(new StreamWriter(Console.OpenStandardOutput(), new UTF8Encoding(false)) { AutoFlush = true });
                Console.SetError(new StreamWriter(Console.OpenStandardError(), new UTF8Encoding(false)) { AutoFlush = true });
            }
            bool extractOnly = false, verifyOnly = false;
            string root = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Youngcraft", "Standalone");
            var forwarded = new List<string>();
            bool literal = false;
            for (int i = 0; i < args.Length; ++i)
            {
                string arg = args[i];
                if (!literal && arg == "--") { literal = true; forwarded.Add(arg); }
                else if (!literal && arg == "--youngcraft-extract-only") extractOnly = true;
                else if (!literal && arg == "--youngcraft-verify-only") verifyOnly = true;
                else if (!literal && arg == "--youngcraft-cache-root")
                {
                    if (++i >= args.Length || String.IsNullOrWhiteSpace(args[i])) throw new ArgumentException("--youngcraft-cache-root requires a directory.");
                    root = args[i];
                }
                else forwarded.Add(arg);
            }
            if (extractOnly && verifyOnly) throw new ArgumentException("Choose extract-only or verify-only.");
            string callerDirectory = Environment.CurrentDirectory;
            root = Path.GetFullPath(root);
            string payloadHash = Encoding.ASCII.GetString(ResourceBytes("YoungcraftStandalone.PayloadHash")).Trim();
            RequireHash(payloadHash);
            byte[] manifestBytes = ResourceBytes("YoungcraftStandalone.Manifest");
            string manifestHash = HashBytes(manifestBytes);
            Dictionary<string, FileRecord> records = ReadManifest(manifestBytes);
            string cache = Path.Combine(root, payloadHash);
            EnsureDirectory(root);
            string lockPath = Path.Combine(root, ".standalone.lock");
            CheckPath(lockPath);
            using (FileStream gate = AcquireLock(lockPath))
            {
                CheckPath(cache);
                if (!Directory.Exists(cache))
                {
                    if (verifyOnly) throw new IOException("The payload has not been extracted: " + cache);
                    Extract(root, cache, payloadHash, manifestBytes, records);
                }
                VerifyCache(cache, manifestHash, records, false);
                // Verification alone never migrates data or updates version metadata.
                if (!verifyOnly) MigrateCustomizations(root, cache, payloadHash, manifestHash, records);
            }
            if (headless)
            {
                Console.Out.WriteLine(cache);
                return 0;
            }
            var info = new ProcessStartInfo(Path.Combine(cache, "Youngcraft.exe"), JoinArguments(forwarded));
            info.UseShellExecute = false;
            // Qt resolves file arguments relative to cwd; runtimes use applicationDirPath.
            info.WorkingDirectory = callerDirectory;
            using (Process child = Process.Start(info))
                if (child == null) throw new IOException("Could not start the cached Youngcraft.exe.");
            return 0;
        }
        catch (Exception error)
        {
            string message = "Youngcraft standalone: " + error.Message;
            if (headless) Console.Error.WriteLine(message);
            else MessageBox.Show(message, "Youngcraft", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }

    private static Stream Resource(string name)
    {
        Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name);
        if (stream == null) throw new InvalidDataException("Missing embedded resource: " + name);
        return stream;
    }
    private static byte[] ResourceBytes(string name)
    {
        using (Stream stream = Resource(name))
        using (var output = new MemoryStream()) { stream.CopyTo(output); return output.ToArray(); }
    }
    private static string HashStream(Stream stream)
    {
        using (SHA256 sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-", "").ToLowerInvariant();
    }
    private static string HashBytes(byte[] bytes)
    {
        using (var stream = new MemoryStream(bytes, false)) return HashStream(stream);
    }
    private static string HashFile(string path)
    {
        CheckPath(path);
        using (FileStream stream = File.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read)) return HashStream(stream);
    }
    private static void RequireHash(string hash)
    {
        if (hash == null || hash.Length != 64) throw new InvalidDataException("Invalid SHA256 value.");
        foreach (char c in hash) if (!Uri.IsHexDigit(c)) throw new InvalidDataException("Invalid SHA256 value.");
    }
    private static bool EqualHash(string a, string b) { return String.Equals(a, b, StringComparison.OrdinalIgnoreCase); }

    // Windows paths must have exactly one representation: no ADS, device names,
    // rooted/UNC paths, traversal, empty segments, trailing dots/spaces, or aliases.
    private static string SafeRelative(string path)
    {
        if (String.IsNullOrEmpty(path)) throw new InvalidDataException("Empty payload path.");
        string normalized = path.Replace('\\', '/');
        foreach (string part in normalized.Split('/'))
        {
            if (part.Length == 0 || part == "." || part == ".." || part.EndsWith(".") || part.EndsWith(" "))
                throw new InvalidDataException("Unsafe payload path: " + path);
            foreach (char c in part)
                if (c < 32 || "<>:\"|?*".IndexOf(c) >= 0) throw new InvalidDataException("Unsafe payload path: " + path);
            string device = part.Split('.')[0].ToUpperInvariant();
            if (device == "CON" || device == "PRN" || device == "AUX" || device == "NUL" || device == "CONIN$" || device == "CONOUT$" ||
                ((device.StartsWith("COM") || device.StartsWith("LPT")) && device.Length == 4 && "123456789\u00b9\u00b2\u00b3".IndexOf(device[3]) >= 0))
                throw new InvalidDataException("Device name in payload: " + path);
        }
        return normalized;
    }
    private static string Destination(string root, string relative)
    {
        string prefix = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        string destination = Path.GetFullPath(Path.Combine(prefix, SafeRelative(relative).Replace('/', Path.DirectorySeparatorChar)));
        if (!destination.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Payload escapes cache.");
        CheckPath(destination);
        return destination;
    }
    // Reject existing junctions/symlinks in every ancestor, including cache roots.
    private static void CheckPath(string path)
    {
        string current = Path.GetFullPath(path);
        while (!String.IsNullOrEmpty(current))
        {
            try
            {
                if ((File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Reparse points are not allowed in the standalone cache: " + current);
            }
            catch (FileNotFoundException) { }
            catch (DirectoryNotFoundException) { }
            current = Path.GetDirectoryName(current);
        }
    }
    private static void EnsureDirectory(string path)
    {
        CheckPath(path);
        Directory.CreateDirectory(path);
        CheckPath(path);
    }
    private static Dictionary<string, FileRecord> ReadManifest(byte[] bytes)
    {
        Manifest manifest;
        using (var reader = new StreamReader(new MemoryStream(bytes, false), Encoding.UTF8, true))
            manifest = Json.Deserialize<Manifest>(reader.ReadToEnd());
        if (manifest == null || manifest.schema != 1 || manifest.files == null || manifest.files.Count == 0)
            throw new InvalidDataException("Invalid package manifest schema.");
        var result = new Dictionary<string, FileRecord>(StringComparer.OrdinalIgnoreCase);
        foreach (FileRecord record in manifest.files)
        {
            if (record == null) throw new InvalidDataException("Null manifest record.");
            string path = SafeRelative(record.path);
            RequireHash(record.sha256);
            if (record.size < 0 || path.Equals(ManifestName, StringComparison.OrdinalIgnoreCase) || result.ContainsKey(path))
                throw new InvalidDataException("Invalid or duplicate manifest file: " + path);
            record.path = path;
            result.Add(path, record);
        }
        if (!result.ContainsKey("Youngcraft.exe")) throw new InvalidDataException("Manifest does not contain Youngcraft.exe.");
        foreach (string path in result.Keys)
        {
            int slash = path.IndexOf('/');
            while (slash >= 0)
            {
                if (result.ContainsKey(path.Substring(0, slash))) throw new InvalidDataException("File/directory collision: " + path);
                slash = path.IndexOf('/', slash + 1);
            }
        }
        return result;
    }
    private static FileStream AcquireLock(string path)
    {
        var timer = Stopwatch.StartNew();
        while (true)
        {
            CheckPath(path);
            try { return new FileStream(path, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None); }
            catch (IOException)
            {
                if (timer.Elapsed.TotalMinutes >= 5) throw new IOException("Timed out waiting for another standalone extraction: " + path);
                Thread.Sleep(100);
            }
        }
    }
    private static void Extract(string root, string cache, string payloadHash, byte[] manifestBytes, Dictionary<string, FileRecord> records)
    {
        using (Stream payload = Resource("YoungcraftStandalone.Payload"))
        {
            if (!EqualHash(HashStream(payload), payloadHash)) throw new InvalidDataException("Embedded ZIP SHA256 mismatch.");
            payload.Position = 0;
            using (var zip = new ZipArchive(payload, ZipArchiveMode.Read, false))
            {
                var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                var entries = new Dictionary<string, ZipArchiveEntry>(StringComparer.OrdinalIgnoreCase);
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    string raw = entry.FullName.Replace('\\', '/');
                    bool directory = raw.EndsWith("/");
                    string path = SafeRelative(directory ? raw.Substring(0, raw.Length - 1) : raw);
                    int type = (entry.ExternalAttributes >> 16) & 0xF000;
                    if (type != 0 && type != 0x8000 && type != 0x4000)
                        throw new InvalidDataException("ZIP contains a symlink or special file: " + path);
                    if ((entry.ExternalAttributes & (int)FileAttributes.ReparsePoint) != 0 || !seen.Add(path))
                        throw new InvalidDataException("Duplicate or reparse ZIP path: " + path);
                    if (directory)
                    {
                        if (entry.Length != 0 || records.ContainsKey(path) || path.Equals(ManifestName, StringComparison.OrdinalIgnoreCase))
                            throw new InvalidDataException("Invalid ZIP directory: " + path);
                        continue;
                    }
                    FileRecord record;
                    if (path.Equals(ManifestName, StringComparison.OrdinalIgnoreCase))
                    {
                        if (entry.Length != manifestBytes.Length) throw new InvalidDataException("ZIP manifest size mismatch.");
                    }
                    else if (!records.TryGetValue(path, out record) || record.size != entry.Length)
                        throw new InvalidDataException("Unlisted ZIP file or size mismatch: " + path);
                    entries.Add(path, entry);
                }
                if (entries.Count != records.Count + 1 || !entries.ContainsKey(ManifestName))
                    throw new InvalidDataException("ZIP is missing manifest files.");
                // Never expose a partial cache. Failed staging directories are retained;
                // neither old versions nor user data are automatically deleted/repaired.
                string stage = Path.Combine(root, ".extracting-" + Guid.NewGuid().ToString("N"));
                EnsureDirectory(stage);
                foreach (var item in entries)
                {
                    string destination = Destination(stage, item.Key);
                    EnsureDirectory(Path.GetDirectoryName(destination));
                    using (Stream input = item.Value.Open())
                    using (FileStream output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                    {
                        long remaining = item.Value.Length;
                        byte[] buffer = new byte[81920];
                        int count;
                        while ((count = input.Read(buffer, 0, buffer.Length)) != 0)
                        {
                            if (count > remaining) throw new InvalidDataException("ZIP entry exceeds declared size.");
                            output.Write(buffer, 0, count);
                            remaining -= count;
                        }
                        if (remaining != 0) throw new InvalidDataException("Truncated ZIP entry: " + item.Key);
                    }
                }
                VerifyCache(stage, HashBytes(manifestBytes), records, true);
                Directory.Move(stage, cache);
            }
        }
    }
    private static bool IsUserFile(string path)
    {
        return path.Equals("youngcraft-server.json", StringComparison.OrdinalIgnoreCase) ||
            (path.StartsWith("lab_fragments/", StringComparison.OrdinalIgnoreCase) && path.EndsWith(".cjson", StringComparison.OrdinalIgnoreCase));
    }
    private static void VerifyCache(string cache, string manifestHash, Dictionary<string, FileRecord> records, bool pristine)
    {
        if (!EqualHash(HashFile(Destination(cache, ManifestName)), manifestHash)) throw new InvalidDataException("Cached manifest SHA256 mismatch: " + cache);
        foreach (FileRecord record in records.Values)
        {
            string path = Destination(cache, record.path);
            // These are data files the user may intentionally edit, never executables.
            if (!pristine && IsUserFile(record.path)) continue;
            if (!File.Exists(path) || new FileInfo(path).Length != record.size || !EqualHash(HashFile(path), record.sha256))
                throw new InvalidDataException("Cached file SHA256 mismatch: " + path + ". Cache preserved; use a different --youngcraft-cache-root to extract a fresh copy.");
        }
        // Reject executable injection through extra DLLs/plugins, while preserving user files.
        foreach (string path in WalkFiles(cache))
        {
            string relative = SafeRelative(path.Substring(cache.TrimEnd('\\').Length + 1));
            string ext = Path.GetExtension(relative);
            if ((ext.Equals(".exe", StringComparison.OrdinalIgnoreCase) || ext.Equals(".dll", StringComparison.OrdinalIgnoreCase)) && !records.ContainsKey(relative))
                throw new InvalidDataException("Unlisted executable in cache: " + path);
        }
    }
    private static IEnumerable<string> WalkFiles(string directory)
    {
        CheckPath(directory);
        foreach (string file in Directory.GetFiles(directory)) { CheckPath(file); yield return file; }
        foreach (string child in Directory.GetDirectories(directory))
            foreach (string file in WalkFiles(child)) yield return file;
    }

    // Qt settings and user-created fragments already live in stable registry/AppData
    // locations. Also carry forward edits made directly beside the cached executable.
    // The ledger records each version's original manifest hash so unchanged old
    // bundled data never overwrites newer bundles. Latest actual customization wins.
    private static void MigrateCustomizations(string root, string cache, string hash, string manifestHash, Dictionary<string, FileRecord> current)
    {
        string ledgerPath = Path.Combine(root, ".standalone-versions.json");
        CheckPath(ledgerPath);
        Versions ledger = File.Exists(ledgerPath) ? Json.Deserialize<Versions>(File.ReadAllText(ledgerPath, Encoding.UTF8)) :
            new Versions { schema = 1, versions = new List<VersionRecord>() };
        if (ledger == null || ledger.schema != 1 || ledger.versions == null) throw new InvalidDataException("Invalid standalone version ledger.");
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var candidates = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (VersionRecord version in ledger.versions)
        {
            if (version == null) throw new InvalidDataException("Invalid version record.");
            RequireHash(version.payloadSha256); RequireHash(version.manifestSha256);
            if (!seen.Add(version.payloadSha256)) throw new InvalidDataException("Duplicate version record.");
            string previous = Path.Combine(root, version.payloadSha256);
            CheckPath(previous);
            if (!Directory.Exists(previous)) continue;
            string previousManifest = Destination(previous, ManifestName);
            if (!EqualHash(HashFile(previousManifest), version.manifestSha256)) throw new InvalidDataException("Previous cache manifest SHA256 mismatch: " + previous);
            var baseline = ReadManifest(File.ReadAllBytes(previousManifest));
            CollectCustomizations(previous, baseline, candidates);
        }
        CollectCustomizations(cache, current, candidates);
        foreach (var item in candidates)
        {
            string destination = Destination(cache, item.Key);
            if (destination.Equals(item.Value, StringComparison.OrdinalIgnoreCase)) continue;
            if (File.Exists(destination) && EqualHash(HashFile(destination), HashFile(item.Value))) continue;
            EnsureDirectory(Path.GetDirectoryName(destination));
            // Atomic replacement avoids exposing a partially copied JSON file.
            string temporary = destination + ".standalone-" + Guid.NewGuid().ToString("N") + ".tmp";
            File.Copy(item.Value, temporary, false);
            File.SetLastWriteTimeUtc(temporary, File.GetLastWriteTimeUtc(item.Value));
            if (File.Exists(destination)) File.Replace(temporary, destination, null);
            else File.Move(temporary, destination);
        }
        if (!seen.Contains(hash))
        {
            ledger.versions.Add(new VersionRecord { payloadSha256 = hash, manifestSha256 = manifestHash });
            string temporary = ledgerPath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            File.WriteAllText(temporary, Json.Serialize(ledger), new UTF8Encoding(false));
            if (File.Exists(ledgerPath)) File.Replace(temporary, ledgerPath, null);
            else File.Move(temporary, ledgerPath);
        }
    }
    private static void CollectCustomizations(string cache, Dictionary<string, FileRecord> baseline, Dictionary<string, string> candidates)
    {
        var files = new List<string>();
        string server = Destination(cache, "youngcraft-server.json");
        if (File.Exists(server)) files.Add(server);
        string fragments = Destination(cache, "lab_fragments");
        if (Directory.Exists(fragments)) files.AddRange(WalkFiles(fragments));
        foreach (string file in files)
        {
            string relative = SafeRelative(file.Substring(cache.TrimEnd('\\').Length + 1));
            if (!IsUserFile(relative)) continue;
            FileRecord original;
            if (baseline.TryGetValue(relative, out original) && EqualHash(HashFile(file), original.sha256)) continue;
            string existing;
            if (!candidates.TryGetValue(relative, out existing) || File.GetLastWriteTimeUtc(file) >= File.GetLastWriteTimeUtc(existing)) candidates[relative] = file;
        }
    }
    // Inverse of CommandLineToArgvW / the Windows C runtime quoting convention.
    private static string QuoteArgument(string argument)
    {
        var result = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in argument)
        {
            if (c == '\\') { ++slashes; continue; }
            if (c == '"') { result.Append('\\', slashes * 2 + 1); result.Append('"'); }
            else { result.Append('\\', slashes); result.Append(c); }
            slashes = 0;
        }
        result.Append('\\', slashes * 2); result.Append('"');
        return result.ToString();
    }
    private static string JoinArguments(List<string> args)
    {
        var result = new List<string>();
        foreach (string arg in args) result.Add(QuoteArgument(arg));
        return String.Join(" ", result.ToArray());
    }
}
