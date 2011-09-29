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

#ifndef SQLSTORAGE_IMPL_H
#define SQLSTORAGE_IMPL_H

#include "ProgressBar.h"
#include "Log.h"
#include "DBCFileLoader.h"

template<class Derived, class T>
template<class S, class D>
void SQLStorageLoaderBase<Derived, T>::convert(uint32 /*field_pos*/, S src, D &dst)
{
    dst = D(src);
}

template<class Derived, class T>
void SQLStorageLoaderBase<Derived, T>::convert_str_to_str(uint32 /*field_pos*/, char const *src, char *&dst)
{
    if(!src)
    {
        dst = new char[1];
        *dst = 0;
    }
    else
    {
        uint32 l = strlen(src) + 1;
        dst = new char[l];
        memcpy(dst, src, l);
    }
}

template<class Derived, class T>
template<class S>
void SQLStorageLoaderBase<Derived, T>::convert_to_str(uint32 /*field_pos*/, S /*src*/, char * & dst)
{
    dst = new char[1];
    *dst = 0;
}

template<class Derived, class T>
template<class D>
void SQLStorageLoaderBase<Derived, T>::convert_from_str(uint32 /*field_pos*/, char const* /*src*/, D& dst)
{
    dst = 0;
}

template<class Derived, class T>
template<class V>
void SQLStorageLoaderBase<Derived, T>::storeValue(V value, T &store, char *record, uint32 field_pos, uint32 &offset)
{
    Derived * subclass = (static_cast<Derived*>(this));
    switch(store.m_dst_format[field_pos])
    {
        case FT_LOGIC:
            subclass->convert(field_pos, value, *((bool*)(&record[offset])) );
            offset+=sizeof(bool);
            break;
        case FT_BYTE:
            subclass->convert(field_pos, value, *((char*)(&record[offset])) );
            offset+=sizeof(char);
            break;
        case FT_INT:
            subclass->convert(field_pos, value, *((uint32*)(&record[offset])) );
            offset+=sizeof(uint32);
            break;
        case FT_FLOAT:
            subclass->convert(field_pos, value, *((float*)(&record[offset])) );
            offset+=sizeof(float);
            break;
        case FT_STRING:
            subclass->convert_to_str(field_pos, value, *((char**)(&record[offset])) );
            offset+=sizeof(char*);
            break;
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

template<class Derived, class T>
void SQLStorageLoaderBase<Derived, T>::storeValue(char const* value, T &store, char *record, uint32 field_pos, uint32 &offset)
{
    Derived * subclass = (static_cast<Derived*>(this));
    switch(store.m_dst_format[field_pos])
    {
        case FT_LOGIC:
            subclass->convert_from_str(field_pos, value, *((bool*)(&record[offset])) );
            offset+=sizeof(bool);
            break;
        case FT_BYTE:
            subclass->convert_from_str(field_pos, value, *((char*)(&record[offset])) );
            offset+=sizeof(char);
            break;
        case FT_INT:
            subclass->convert_from_str(field_pos, value, *((uint32*)(&record[offset])) );
            offset+=sizeof(uint32);
            break;
        case FT_FLOAT:
            subclass->convert_from_str(field_pos, value, *((float*)(&record[offset])) );
            offset+=sizeof(float);
            break;
        case FT_STRING:
            subclass->convert_str_to_str(field_pos, value, *((char**)(&record[offset])) );
            offset+=sizeof(char*);
            break;
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

template<class Derived, class T>
void SQLStorageLoaderBase<Derived, T>::Load(T &store, bool error_at_empty /*= true*/)
{
    Field *fields = NULL;
    QueryResult *result  = WorldDatabase.PQuery("SELECT MAX(%s) FROM %s", store.EntryFieldName(), store.GetTableName());
    if(!result)
    {
        sLog.outError("Error loading %s table (not exist?)\n", store.GetTableName());
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Stop server at loading non exited table or not accessable table
    }

    uint32 maxRecordId = (*result)[0].GetUInt32()+1;
    uint32 recordCount = 0;
    uint32 recordsize = 0;
    delete result;

    result = WorldDatabase.PQuery("SELECT COUNT(*) FROM %s", store.GetTableName());
    if(result)
    {
        fields = result->Fetch();
        recordCount = fields[0].GetUInt32();
        delete result;
    }

    result = WorldDatabase.PQuery("SELECT * FROM %s", store.GetTableName());

    if(!result)
    {
        if (error_at_empty)
            sLog.outError("%s table is empty!\n", store.GetTableName());
        else
            sLog.outString("%s table is empty!\n", store.GetTableName());
        return;
    }

    if (store.FieldCount != result->GetFieldCount())
    {
        sLog.outError("Error in %s table, probably sql file format was updated (there should be %d fields in sql).\n", store.GetTableName(), store.FieldCount);
        delete result;
        Log::WaitBeforeContinueIfNeed();
        exit(1);                                            // Stop server at loading broken or non-compatible table.
    }

    //get struct size
    for(uint32 x = 0; x < store.FieldCount; ++x)
    {
        switch(store.m_dst_format[x])
        {
            case FT_LOGIC:
                recordsize += sizeof(bool);   break;
            case FT_BYTE:
                recordsize += sizeof(char);   break;
            case FT_INT:
                recordsize += sizeof(uint32); break;
            case FT_FLOAT:
                recordsize += sizeof(float);  break;
            case FT_STRING:
                recordsize += sizeof(char*);  break;
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

    store.prepareToLoad(maxRecordId, recordCount, recordsize);

    BarGoLink bar(store.RecordCount);
    do
    {
        bar.step();
        fields = result->Fetch();
        char* record = store.createRecord(fields[0].GetUInt32());
        uint32 offset = 0;
        for (uint32 field_pos = 0; field_pos < store.FieldCount; ++field_pos)
        {
            switch(store.m_src_format[field_pos])
            {
                case FT_LOGIC:
                    storeValue((bool)(fields[field_pos].GetUInt32() > 0), store, record, field_pos, offset); break;
                case FT_BYTE:
                    storeValue((char)fields[field_pos].GetUInt8(), store, record, field_pos, offset); break;
                case FT_INT:
                    storeValue((uint32)fields[field_pos].GetUInt32(), store, record, field_pos, offset); break;
                case FT_FLOAT:
                    storeValue((float)fields[field_pos].GetFloat(), store, record, field_pos, offset); break;
                case FT_STRING:
                    storeValue((char const*)fields[field_pos].GetString(), store, record, field_pos, offset); break;
                case FT_NA:
                case FT_NA_BYTE:
                    break;
                case FT_IND:
                case FT_SORT:
                    assert(false && "SQL storage not have sort field types");
                    break;
                default:
                    assert(false && "unknown format character");
            }
        }
    }while( result->NextRow() );

    delete result;
}

#endif
