#ifndef _DROPFOLDER_MESSAGE_H_
#define _DROPFOLDER_MESSAGE_H_

#define MESSAGE_MAX_FILENAME_SIZE 256

enum class MessageType {
    DELETE_FILE,
    UPDATE_FILE,
    USERNAME,
    USERNAME_NOSYNC,
    REQUEST_FILE,
    NO_SUCH_FILE,
    REQUEST_FILE_LIST,
    FILE_LIST,
    DROP,
    GROUP_INFO_REQUEST,
    GROUP_INFO_REPLY
};

struct Message {
    MessageType type;
    char filename[MESSAGE_MAX_FILENAME_SIZE]; // user name for USERNAME and USERNAME_NOSYNC
    unsigned int file_length;
    char bytes[];
};

#endif //  _DROPFOLDER_MESSAGE_H_

