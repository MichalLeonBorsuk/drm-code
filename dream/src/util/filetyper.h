#ifndef FILETYPER_H
#define FILETYPER_H

#include <string>

class FileTyper
{
public:
    enum type { pcap, file_framing, raw_af, raw_pft, pcm };
    FileTyper();
    static type resolve(const std::string& s);
    static bool is_rxstat(type t) { return t != pcm; }
};

#endif // FILETYPER_H
