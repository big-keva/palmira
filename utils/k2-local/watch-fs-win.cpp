# include "watch-fs.hpp"
# include <moonycode/codes.h>
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <functional>
# include <stdexcept>
# include <vector>
# include <thread>
# include <string>
# include <map>
# include <atomic>

namespace palmira {
namespace k2_find {

  struct WatchInfo
  {
    HANDLE            directoryHandle = INVALID_HANDLE_VALUE;
    std::wstring      path;
    bool              recursive;
    OVERLAPPED        overlapped;
    std::vector<BYTE> buffer;
    bool              pendingIO = false;
  };

  struct WatchDir::WatchData
  {
    std::map<HANDLE, WatchInfo> watches;
    std::vector<HANDLE>         handles;
    HANDLE                      stopEvent;
    std::atomic<bool>           stopFlag{false};

    WatchData()
    {
      // Создаем событие остановки и добавляем его первым элементом
      stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (stopEvent != NULL)
      {
        handles.push_back(stopEvent);
      }
    }

   ~WatchData()
    {
      // Закрываем все handles
      for (auto& pair : watches)
      {
        if (pair.second.directoryHandle != INVALID_HANDLE_VALUE)
        {
          CancelIo(pair.second.directoryHandle);
          CloseHandle(pair.second.directoryHandle);
        }
        if (pair.second.overlapped.hEvent != NULL)
        {
          CloseHandle(pair.second.overlapped.hEvent);
        }
      }

      if (stopEvent != NULL)
      {
        CloseHandle(stopEvent);
      }
    }
  };

  WatchDir::WatchDir( NotifyFn fn ):
    watchPtr( std::make_shared<WatchData>() ),
    notifyFn( std::move( fn ) )
  {
    if ( watchPtr->stopEvent == NULL )
    {
      throw std::runtime_error("Failed to create stop event");
    }

    watchThr = std::thread( &WatchDir::DirWatch, this );
  }

  WatchDir::~WatchDir()
  {
    if ( watchPtr )
    {
      watchPtr->stopFlag = true;

      // Сигнализируем событию для выхода из ожидания
      if (watchPtr->stopEvent != NULL)
      {
        SetEvent(watchPtr->stopEvent);
      }
    }

    if (watchThr.joinable())
    {
      watchThr.join();
    }
  }

  void  WatchDir::AddWatch(std::string path, bool withSubdirectories)
  {
    WatchInfo watchInfo;

    if ( !watchPtr )
      return;

    // Создаем структуру для нового watcher'а
    watchInfo.path = std::move( codepages::mbcstowide( codepages::codepage_utf8, path ) );
    watchInfo.recursive = withSubdirectories;
    watchInfo.buffer.resize(64 * 1024);
    ZeroMemory(&watchInfo.overlapped, sizeof(OVERLAPPED));

    watchInfo.directoryHandle = CreateFileW(
      watchInfo.path.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      NULL );

    if ( watchInfo.directoryHandle == INVALID_HANDLE_VALUE )
      throw std::invalid_argument( "invalid directory name" );

    // Создаем событие для этого watcher'а
    watchInfo.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (watchInfo.overlapped.hEvent == NULL)
    {
      CloseHandle( watchInfo.directoryHandle );
      throw std::runtime_error("Failed to create watch event");
    }

    // Добавляем в карту и в массив handles
    watchPtr->watches[watchInfo.overlapped.hEvent] = std::move(watchInfo);
    watchPtr->handles.push_back( watchInfo.overlapped.hEvent );

    // Запускаем асинхронное чтение для нового watcher'а
    WatchInfo& newWatch = watchPtr->watches[watchInfo.overlapped.hEvent];

    DWORD bytesReturned = 0;
    BOOL success = ReadDirectoryChangesW(
      newWatch.directoryHandle,
      newWatch.buffer.data(),
      static_cast<DWORD>(newWatch.buffer.size()),
      newWatch.recursive,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES
        | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
      &bytesReturned,
      &newWatch.overlapped,
      NULL );

    if (!success)
    {
      // Удаляем неудавшийся watcher
      watchPtr->handles.pop_back();
      CloseHandle(newWatch.overlapped.hEvent);
      CloseHandle(newWatch.directoryHandle);
      watchPtr->watches.erase(watchInfo.overlapped.hEvent);
      throw std::runtime_error("Failed to start directory monitoring");
    }

    newWatch.pendingIO = true;

    // Сигнализируем для перезапуска ожидания в потоке
    SetEvent(watchPtr->stopEvent);
  }

  void  WatchDir::DirWatch()
  {
    if ( !watchPtr || !notifyFn )
      return;

    while ( !watchPtr->stopFlag )
    {
      if ( watchPtr->handles.size() <= 1 )  // Только stopEvent
      {
        // Ждем, пока добавят хотя бы один каталог
        WaitForSingleObject(watchPtr->stopEvent, 100);
        ResetEvent(watchPtr->stopEvent);
        continue;
      }

      // Ожидаем любого из событий (массив handles уже подготовлен)
      DWORD waitResult = WaitForMultipleObjects(
        static_cast<DWORD>(watchPtr->handles.size()),
        watchPtr->handles.data(),
        FALSE,
        INFINITE);

      if ( watchPtr->stopFlag )
        break;

      if ( waitResult == WAIT_OBJECT_0 )
      {
        // Событие остановки или добавления нового watcher'а
        ResetEvent(watchPtr->stopEvent);
        continue;
      }

      if ( waitResult >= WAIT_OBJECT_0 + 1 && waitResult < WAIT_OBJECT_0 + watchPtr->handles.size() )
      {
        // Сохраняем индекс до любых модификаций вектора
        size_t eventIndex = waitResult - WAIT_OBJECT_0;
        HANDLE signaledEvent = watchPtr->handles[eventIndex];
        auto it = watchPtr->watches.find(signaledEvent);

        if ( it != watchPtr->watches.end() )
        {
          WatchInfo& watchInfo = it->second;

          DWORD bytesReturned = 0;
          if ( !GetOverlappedResult(watchInfo.directoryHandle, &watchInfo.overlapped, &bytesReturned, FALSE) )
          {
            // Ошибка - удаляем этот watcher
            watchPtr->handles.erase( watchPtr->handles.begin() + eventIndex );
            CloseHandle(watchInfo.overlapped.hEvent);
            CloseHandle(watchInfo.directoryHandle);
            watchPtr->watches.erase(it);
            continue;
          }

          watchInfo.pendingIO = false;

          if ( bytesReturned > 0 )
          {
            // Обрабатываем полученные изменения
            FILE_NOTIFY_INFORMATION* notifyInfo =
              reinterpret_cast<FILE_NOTIFY_INFORMATION*>(watchInfo.buffer.data());

            do
            {
              auto  fullPath = codepages::widetombcs( codepages::codepage_utf8, watchInfo.path
                + std::wstring( notifyInfo->FileName, notifyInfo->FileNameLength / sizeof(WCHAR) ) );

              switch (notifyInfo->Action)
              {
                case FILE_ACTION_ADDED:
                case FILE_ACTION_RENAMED_NEW_NAME:
                  notifyFn( create_file, fullPath );
                  break;

                case FILE_ACTION_MODIFIED:
                  notifyFn( modify_file, fullPath );
                  break;

                case FILE_ACTION_REMOVED:
                case FILE_ACTION_RENAMED_OLD_NAME:
                  notifyFn( delete_file, fullPath );
                  break;
              }

              if (notifyInfo->NextEntryOffset == 0)
                break;

              notifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(notifyInfo) + notifyInfo->NextEntryOffset);

            } while (!watchPtr->stopFlag);
          }

          // Сбрасываем событие и перезапускаем мониторинг
          ResetEvent(signaledEvent);

          // Перезапускаем мониторинг для этого каталога
          BOOL success = ReadDirectoryChangesW(
            watchInfo.directoryHandle,
            watchInfo.buffer.data(),
            DWORD(watchInfo.buffer.size()),
            watchInfo.recursive,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES
              | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
            &(bytesReturned = 0),
            &watchInfo.overlapped,
            NULL );

          if (success)  watchInfo.pendingIO = true;
            else
          {
            // Ошибка - удаляем этот watcher
            watchPtr->handles.erase( watchPtr->handles.begin() + eventIndex );
            CloseHandle(watchInfo.overlapped.hEvent);
            CloseHandle(watchInfo.directoryHandle);
            watchPtr->watches.erase(it);
          }
        } else watchPtr->handles.erase( watchPtr->handles.begin() + eventIndex ); // Watcher не найден в карте - удаляем из handles
      }
        else
      if (waitResult == WAIT_FAILED)
      {
        // Ошибка WaitForMultipleObjects - небольшая пауза и продолжаем
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

}}
