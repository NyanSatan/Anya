#ifndef ANYA_LOG_H
#define ANYA_LOG_H

#define ANSI_START   "\x1b["
#define ANSI_DELIM   ";"
#define ANSI_BOLD    "1"
#define ANSI_RED     "31"
#define ANSI_GREEN   "32"
#define ANSI_YELLOW  "33"
#define ANSI_RESET   "0"
#define ANSI_END     "m"

#define SEQ_BOLD    ANSI_START ANSI_BOLD ANSI_END
#define SEQ_RESET   ANSI_START ANSI_RESET ANSI_END

#define BREAK   "\n"

#define ANYA_MISC(__format, ...)    printf(__format BREAK, ##__VA_ARGS__);
#define ANYA_INFO(__format, ...)    printf(ANSI_START ANSI_BOLD ANSI_END __format ANSI_START ANSI_RESET ANSI_END BREAK, ##__VA_ARGS__);
#define ANYA_SUCCESS(__format, ...) printf(ANSI_START ANSI_GREEN ANSI_DELIM ANSI_BOLD ANSI_END __format ANSI_START ANSI_RESET ANSI_END BREAK, ##__VA_ARGS__);
#define ANYA_WARNING(__format, ...) printf(ANSI_START ANSI_YELLOW ANSI_DELIM ANSI_BOLD ANSI_END __format ANSI_START ANSI_RESET ANSI_END BREAK, ##__VA_ARGS__);
#define ANYA_ERROR(__format, ...)   printf(ANSI_START ANSI_RED ANSI_DELIM ANSI_BOLD ANSI_END __format ANSI_START ANSI_RESET ANSI_END BREAK, ##__VA_ARGS__);

#endif
