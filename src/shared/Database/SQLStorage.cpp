/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SQLStorage.h"
#include "SQLStorageImpl.h"

void deallocateRecords(char *& records, const char * format, uint32 iNumFields, uint32 recordCount, uint32 recordSize)
{
    if (!records)
        return;

    uint32 offset = 0;
    for(uint32 x = 0; x < iNumFields; ++x)
    {
        switch(format[x])
        {
        case FT_LOGIC:
            offset += sizeof(bool);   break;
        case FT_BYTE:
            offset += sizeof(char);   break;
        case FT_INT:
            offset += sizeof(uint32); break;
        case FT_FLOAT:
            offset += sizeof(float);  break;
        case FT_STRING:
            {
                for (uint32 recordItr = 0; recordItr < recordCount; ++recordItr)
                    delete [] (records + recordItr*recordSize + offset);

                offset += sizeof(char*);
                break;
            }
        case FT_NA:
        case FT_NA_BYTE:
            break;
        case FT_IND:
        case FT_SORT:
            assert(false && "SQL storage not have sort field types");
            break;
        default:
            assert(false && "unknown format character");
            break;
        }
    }
    delete [] records;
    records = NULL;
}

void SQLStorage::EraseEntry(uint32 id)
{
    m_Index[id] = NULL;
}

void SQLStorage::Free ()
{
    deallocateRecords(m_data,m_dst_format,FieldCount,RecordCount,m_recordSize);
    delete [] m_Index;
    m_Index = NULL;
}

void SQLStorage::Load()
{
    Free();
    SQLStorageLoader loader;
    loader.Load(*this);
}

void SQLStorage::prepareToLoad(uint32 maxEntry, uint32 recordCount, uint32 recordSize)
{
    MaxEntry = maxEntry;
    delete[] m_Index;
    m_Index = new char*[maxEntry];
    memset(m_Index, 0, maxEntry * sizeof(char*));

    delete[] m_data;
    m_data = new char[recordCount*recordSize];
    memset(m_data, 0, recordCount*recordSize);
    m_recordSize = recordSize;
    RecordCount = 0;
}

char* SQLStorage::createRecord(uint32 recordId)
{
    m_Index[recordId] = &m_data[RecordCount * m_recordSize];
    ++RecordCount;
    return m_Index[recordId];
}

void SQLStorage::init(const char * _entry_field, const char * sqlname)
{
    m_entry_field = _entry_field;
    m_tableName = sqlname;
    m_data = NULL;
    m_Index = NULL;
    FieldCount = strlen(m_src_format);

    MaxEntry = 0;
    RecordCount = 0;
    m_recordSize = 0;
}

//////////////////////////////////////////////////////////////////////////

void SQLHashStorage::Load()
{
    Free();
    SQLHashStorageLoader loader;
    loader.Load(*this);
}

void SQLHashStorage::Free()
{
    m_records.clear();
    deallocateRecords(m_data,m_dst_format,FieldCount,RecordCount,m_recordSize);
}

void SQLHashStorage::prepareToLoad(uint32 maxRecordId, uint32 recordCount, uint32 recordSize)
{
    MaxEntry = maxRecordId;
    delete[] m_data;
    m_data = new char[recordCount*recordSize];
    memset(m_data, 0, recordCount*recordSize);
    m_recordSize = recordSize;
    RecordCount = 0;
}

char* SQLHashStorage::createRecord(uint32 recordId)
{
    char * record = &m_data[RecordCount * m_recordSize];
    ++RecordCount;
    m_records.insert(RecordMap::value_type(recordId,record));
    return record;
}

void SQLHashStorage::init(const char * _entry_field, const char * sqlname)
{
    m_entry_field = _entry_field;
    m_tableName = sqlname;
    m_data = NULL;
    FieldCount = strlen(m_src_format);

    MaxEntry = 0;
    RecordCount = 0;
    m_recordSize = 0;
}

void SQLHashStorage::EraseEntry(uint32 id)
{
    // do not erase from m_records
    RecordMap::iterator it = m_records.find(id);
    if (it != m_records.end())
        it->second = NULL;
}
