/*
 * moFileReader - A simple .mo-File-Reader
 * Copyright (C) 2009 Domenico Gentner (scorcher24@gmail.com)
 * All rights reserved.                          
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. The names of its contributors may not be used to endorse or promote 
 *      products derived from this software without specific prior written 
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "../include/moFileReader.h"
#include <iostream>

MO_BEGIN_NAMESPACE

// Small Helpers
bool GetPoEditorString(const std::string _line, const std::string& _searchee, std::string &_out);
void MakeHtmlConform(std::string& _inout);

unsigned long moFileReader::SwapBytes(unsigned long _in) 
{
    unsigned long b0 = (_in >> 0) & 0xff;
    unsigned long b1 = (_in >> 8) & 0xff;
    unsigned long b2 = (_in >> 16) & 0xff;
    unsigned long b3 = (_in >> 24) & 0xff;

    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

const std::string& moFileReader::GetErrorDescription() const
{
    return m_error;
}

void moFileReader::ClearTable()
{
    m_lookup.clear();
}

unsigned int moFileReader::GetNumStrings() const
{
    return m_lookup.size();
}

std::string moFileReader::Lookup( const char* _id ) const
{
    if ( m_lookup.size() <= 0) return _id;
    moLookupList::const_iterator _iterator = m_lookup.find(_id);    

    if ( _iterator == m_lookup.end() )
    {
        return _id;
    }
    return _iterator->second;
}

moFileReader::eErrorCode moFileReader::ReadFile( const char* _filename )
{    
    // Creating a file-description.
    moFileInfo moInfo;

    // Reference to the List inside moInfo.
    moFileInfo::moTranslationPairList& _TransPairInfo = moInfo.m_translationPairInformation;

    // Opening the file.
    std::ifstream _stream( _filename, std::ios_base::binary | std::ios_base::in );
    if ( !_stream.is_open() )
    {
        m_error = std::string("Cannot open File ") + std::string(_filename);
        return moFileReader::EC_FILENOTFOUND;
    }

    // Read in all the 4 bytes of fire-magic, offsets and stuff...
    _stream.read((char*)&moInfo.m_magicNumber, 4);
    _stream.read((char*)&moInfo.m_fileVersion, 4);
    _stream.read((char*)&moInfo.m_numStrings, 4);
    _stream.read((char*)&moInfo.m_offsetOriginal, 4);
    _stream.read((char*)&moInfo.m_offsetTranslation, 4);
    _stream.read((char*)&moInfo.m_sizeHashtable, 4);
    _stream.read((char*)&moInfo.m_offsetHashtable, 4);

    if ( _stream.bad() )
    {
        _stream.close();
        m_error = "Stream bad during reading. The .mo-file seems to be invalid or has bad descriptions!";
        return moFileReader::EC_FILEINVALID;
    }

    // Checking the Magic Number
    if ( _MagicNumber != moInfo.m_magicNumber )
    {
        if ( _MagicReversed != moInfo.m_magicNumber )
        {
            m_error = "The Magic Number does not match in all cases!";
            return moFileReader::EC_MAGICNUMBER_NOMATCH;
        }
        else
        {
            moInfo.m_reversed = true;
            m_error = "Magic Number is reversed. We do not support this yet!";
            return moFileReader::EC_MAGICNUMBER_REVERSED;
        }
    }  
    
    // Now we search all Length & Offsets of the original strings
    for ( int i = 0; i < moInfo.m_numStrings; i++ )
    {
        moTranslationPairInformation _str;        
        _stream.read((char*)&_str.m_orLength, 4);
        _stream.read((char*)&_str.m_orOffset, 4);
        if ( _stream.bad() )
        {
            _stream.close();
            m_error = "Stream bad during reading. The .mo-file seems to be invalid or has bad descriptions!";
            return moFileReader::EC_FILEINVALID;
        }

        _TransPairInfo.push_back(_str);
    }

    // Get all Lengths & Offsets of the translated strings
    // Be aware: The Descriptors already exist in our list, so we just mod. refs from the deque.
    for ( int i = 0; i < moInfo.m_numStrings; i++ )
    {
        moTranslationPairInformation& _str = _TransPairInfo[i];
        _stream.read((char*)&_str.m_trLength, 4);
        _stream.read((char*)&_str.m_trOffset, 4);
        if ( _stream.bad() )
        {
            _stream.close();
            m_error = "Stream bad during reading. The .mo-file seems to be invalid or has bad descriptions!";
            return moFileReader::EC_FILEINVALID;
        }
    }

    // Normally you would read the hash-table here, but we don't use it. :)

    // Now to the interesting part, we read the strings-pairs now
    for ( int i = 0; i < moInfo.m_numStrings; i++)
    {
        // We need a length of +1 to catch the trailing \0.
        int orLength = _TransPairInfo[i].m_orLength+1;
        int trLength = _TransPairInfo[i].m_trLength+1;

        int orOffset = _TransPairInfo[i].m_orOffset;
        int trOffset = _TransPairInfo[i].m_trOffset;

        // Original
        char* _original  = new char[orLength];
        memset(_original, 0, sizeof(char)*orLength);

        _stream.seekg(orOffset);
        _stream.read(_original, orLength);

        if ( _stream.bad() )
        {
            m_error = "Stream bad during reading. The .mo-file seems to be invalid or has bad descriptions!";
            return moFileReader::EC_FILEINVALID;
        }
        
        // Translation
        char* _translation = new char[trLength];
        memset(_translation, 0, sizeof(char)*trLength);

        _stream.seekg(trOffset);
        _stream.read(_translation, trLength);

        if ( _stream.bad() )
        {
            m_error = "Stream bad during reading. The .mo-file seems to be invalid or has bad descriptions!";
            return moFileReader::EC_FILEINVALID;
        }

        // Store it in the map.    
        m_lookup[std::string(_original)] = std::string(_translation);

        // Cleanup...
        delete _original;
        delete _translation;
    }

    // Done :)
    _stream.close();
    return moFileReader::EC_SUCCESS;
}



moFileReader::eErrorCode moFileReader::ExportAsHTML(std::string _infile, std::string _filename, std::string _css )
{
    // Read the file
    moFileReader reader;
    moFileReader::eErrorCode r = reader.ReadFile(_infile.c_str()) ;
    if ( r != moFileReader::EC_SUCCESS )
    {
        return r;
    }
    if ( reader.m_lookup.empty() )
    {
        return moFileReader::EC_TABLEEMPTY;
    }    

    // Beautify Output
    std::string fname;
    unsigned int pos = _infile.find_last_of(moPATHSEP);
    if ( pos != std::string::npos )
    {
        fname = _infile.substr( pos+1, _infile.length() );
    }
    else
    {
        fname = _infile;
    }

    std::string htmlfile(_filename);
    if (htmlfile.empty())
    {
        htmlfile = _infile + std::string(".html");
    }   

    // Ok, now prepare output.
    std::ofstream _stream(htmlfile.c_str());
    if ( _stream.is_open() ) 
    {
        _stream << "<!DOCTYPE HTML PUBLIC \"- //W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">" << std::endl;
        _stream << "<html><head><style type=\"text/css\">\n" << std::endl;
        _stream << _css << std::endl;
        _stream << "</style>" << std::endl;
        _stream << "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">" << std::endl;
        _stream << "<title>.mo-File-Export</title></head>" << std::endl;
        _stream << "<body>" << std::endl;
        _stream << "<center>" <<std::endl;
        _stream << "<h1>" << fname << "</h1>" << std::endl;
        _stream << "<table border=\"1\"><th colspan=\"2\">Project Info</th>" << std::endl;

        std::string projectname;
        std::stringstream parsee;
        parsee << reader.Lookup("");
     
        while ( !parsee.eof() )
        {
            std::string line;
            char buffer[1024];
            parsee.getline(buffer, 1024);
            line = buffer;
            if ( !line.empty() )
            {
                std::string _out;
                if ( GetPoEditorString(line, "Project-Id-Version:", _out) )
                {
                    _stream << "<tr><td>" << "Project-Id-Version" << "</td><td>" <<  _out << "</td></tr>" << std::endl;
                }
                if ( GetPoEditorString(line, "Report-Msgid-Bugs-To:", _out) )
                {
                    _stream << "<tr><td>" << "Report-Msgid-Bugs-To" << "</td><td>" <<  _out << "</td></tr>" << std::endl;
                }
                if ( GetPoEditorString(line, "Last-Translator:", _out) )
                {
                    _stream << "<tr><td>" << "Last-Translator" << "</td><td>" <<  _out << "</td></tr>" << std::endl;
                }
                if ( GetPoEditorString(line, "Language-Team", _out) )
                {
                    _stream << "<tr><td>" << "Language-Team" << "</td><td>" <<  _out << "</td></tr>" << std::endl;
                }
            }
        }
        _stream << "</table>" << std::endl;
        _stream << "<hr noshade/>" << std::endl;


        // Now output the content
        _stream << "<table border=\"1\"><th colspan=\"2\">Content</th>" << std::endl;
        moLookupList::const_iterator _it = reader.m_lookup.begin();
        _it++; // Skip the first entry, its the list we parsed above.
        for ( ; _it != reader.m_lookup.end(); _it++)
        {
            _stream << "<tr><td>" << _it->first << "</td><td>" <<  _it->second << "</td></tr>" << std::endl;
        }
        _stream << "</table><br/>" << std::endl;

        _stream << "</center>" << std::endl;
        _stream << "<div class=\"copyleft\">File generated by <a href=\"http://mofilereader.googlecode.com\" target=\"_blank\">moFileReaderSDK</a></div>" << std::endl;        
        _stream << "</body></html>" << std::endl;
        _stream.close();
    }
    else
    {
        return moFileReader::EC_FILEINVALID;
    }
   
    return moFileReader::EC_SUCCESS;
}


moFileReaderSingleton& moFileReaderSingleton::GetInstance()
{
    static moFileReaderSingleton _theoneandonly;
    return _theoneandonly;
} 


moFileReaderSingleton::moFileReaderSingleton(const moFileReaderSingleton& )
{
}

moFileReaderSingleton::moFileReaderSingleton()
{
}

bool IsEmpty(const std::string& _in)
{
    if ( _in.length() == 0 )
    {
        return true;
    }
    if ( _in.length() == 1 && _in[0] == ' ')
    {
        return true;
    }
    return false;
}

bool GetPoEditorString(const std::string _line, const std::string& _searchee, std::string &_out)
{
    if ( _line.find(_searchee) != std::string::npos )
    {
        _out = _line.substr(_line.find_first_of(":")+1, _line.length());
        if ( !IsEmpty(_out) )            
        {
            MakeHtmlConform(_out);
            return true;
        }
    }
    return false;
}

// Replaces < with ( to satisfy html-rules.
void MakeHtmlConform(std::string& _inout)
{
    std::string temp = _inout;
    for ( unsigned int i = 0; i < temp.length(); i++)
    {
        if ( temp[i] == '>')
        {
            _inout.replace(i, 1, ")");
        }
        if ( temp[i] == '<' )
        {
            _inout.replace(i, 1, "(");
        }
    }    
}

MO_END_NAMESPACE
