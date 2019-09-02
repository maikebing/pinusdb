#pragma once
#include "internal.h"
#include "port/os_file.h"
#include "util/arena.h"
#include "util/ker_list.h"
#include "query/result_filter.h"
#include "storage/normal_part_idx.h"
#include "storage/normal_data_page.h"
#include "util/ref_util.h"
#include <atomic>

typedef struct _DataFileMeta
{
  char headStr_[16];            
  uint32_t pageSize_;
  uint32_t fieldCnt_;
  uint32_t partCode_;
  uint32_t tableType_;

  FieldInfoFormat fieldRec_[PDB_TABLE_MAX_FIELD_COUNT];
  char padding_[10460]; //640
  uint32_t crc_;
}DataFileMeta;

class DataPart
{
public:
  DataPart()
  {
    refCnt_ = 0;
    bgDayTs_ = 0;
    edDayTs_ = 0;
  }

  virtual ~DataPart() { }

  virtual void Close() = 0;
  virtual PdbErr_t RecoverDW(const char* pPageBuf) = 0;
  virtual PdbErr_t InsertRec(int64_t devId, int64_t tstamp, 
    bool replace, const uint8_t* pRec, size_t recLen) = 0;

  PdbErr_t QueryAsc(const std::list<int64_t>& devIdList, int64_t bgTs, int64_t edTs,
    int* pTypes, size_t fieldCnt, IResultFilter* pResult, uint64_t timeOut);
  PdbErr_t QueryDesc(const std::list<int64_t>& devIdList, int64_t bgTs, int64_t edTs,
    int* pTypes, size_t fieldCnt, IResultFilter* pResult, uint64_t timeOut);
  PdbErr_t QueryFirst(std::list<int64_t>& devIdList, int64_t bgTs, int64_t edTs,
    int* pTypes, size_t fieldCnt, IResultFilter* pResult, uint64_t timeOut);
  PdbErr_t QueryLast(std::list<int64_t>& devIdList, int64_t bgTs, int64_t edTs,
    int* pTypes, size_t fieldCnt, IResultFilter* pResult, uint64_t timeOut);

  virtual PdbErr_t UnMap() { return PdbE_OK; }

  virtual PdbErr_t DumpToCompPart(const char* pDataPath) { return PdbE_OK; }
  virtual bool SwitchToReadOnly() { return true; }
  virtual bool IsPartReadOnly() const { return true; }
  virtual bool IsNormalPart() const { return false; }
  virtual PdbErr_t SyncDirtyPages(bool syncAll, OSFile* pDwFile) { return PdbE_FILE_READONLY; }
  virtual PdbErr_t AbandonDirtyPages() { return PdbE_OK; }
  virtual size_t GetDirtyPageCnt() { return 0; }

  void AddRef() { refCnt_.fetch_add(1); }
  void MinusRef() { refCnt_.fetch_sub(1); }
  unsigned int GetRefCnt() { return refCnt_; }
  int32_t GetPartCode() const { return static_cast<int32_t>(bgDayTs_ / MillisPerDay); }
  std::string GetIdxPath() const { return idxPath_; }
  std::string GetDataPath() const { return dataPath_; }

protected:
  virtual PdbErr_t QueryDevAsc(int64_t devId, void* pQueryParam,
    IResultFilter* pResult, uint64_t timeOut, bool queryFirst, bool* pIsAdd) = 0;
  virtual PdbErr_t QueryDevDesc(int64_t devId, void* pQueryParam,
    IResultFilter* pResult, uint64_t timeOut, bool queryLast, bool* pIsAdd) = 0;

  virtual void* InitQueryParam(int* pTypes, size_t valCnt, int64_t bgTs, int64_t edTs) = 0;
  virtual void ClearQueryParam(void* pQueryParam) = 0;

protected:
  std::atomic_int refCnt_;
  
  std::string idxPath_;
  std::string dataPath_;

  int64_t bgDayTs_;
  int64_t edDayTs_;
};
