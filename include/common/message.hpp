#ifndef _DROPFOLDER_MESSAGE_H_
#define _DROPFOLDER_MESSAGE_H_

#define MESSAGE_MAX_FILENAME_SIZE 256

enum class MessageType {
    DELETE_FILE,
    UPDATE_FILE,
    USERNAME,
    REQUEST_FILE,
    NO_SUCH_FILE,
    LIST_DIR
};

struct Message {
    MessageType type;
    char filename[MESSAGE_MAX_FILENAME_SIZE]; // user name for USERNAME
    unsigned int file_length;
    char bytes[];
};

#endif //  _DROPFOLDER_MESSAGE_H_

