# Introduction

The settings storage can be used to store and retrieve various configurable parameters. This storage is a RAM-based map (for fast access), but one or more files can be used too (so values can persist across system reboots).

This is a thread-safe implementation. Different threads may access the settings simultaneously.

# Setting Definition

Each setting is a key/value pair.

The key is always a string, while there are many value types supported.
 
## Strings

All strings, whether they represent a key or a string value, must use a specific format. They must be ASCII, NULL-terminated strings, and they are always case-sensitive. They must obey to the following rules:

- they cannot start with a number.
- they cannot contain the characters '=', ';'.
- they cannot contain the characters '\n', '\r'.

## Setting Type

Since each setting has its own type, it is the user's responsibility to access the setting using the correct type. Some "casts" are possible (e.g. between bool and int), but generally reading with the wrong type will lead to failure.

The following types are currently supported.

- String.
- int
- bool
- ip address
- float

### Setting Storage Size

Kconfig is used to determine the size of the various fields used:

- <code>CONFIG_SYSTEM_SETTINGS_MAP_SIZE&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</code>the total number settings allowed
- <code>CONFIG_SYSTEM_SETTINGS_MAX_STORAGES&nbsp;</code>the number of storage files that can be used
- <code>CONFIG_SYSTEM_SETTINGS_VALUE_SIZE&nbsp;&nbsp;&nbsp;</code>the storage size of a STRING value
- <code>CONFIG_SYSTEM_SETTINGS_KEY_SIZE&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</code>the size of the KEY field
- <code>CONFIG_SYSTEM_SETTINGS_MAX_FILENAME&nbsp;</code>the maximum filename size

# Signal

A POSIX signal can be chosen via Kconfig and is used to send notifications when a setting has been changed. <CONFIG_SYSTEM_SETTINGS_MAX_SIGNALS> is used to determine the maximum number of signals that can be registered for the settings storage functions.

# Files

There are also various types of files that can be used. Each file type is using a different data format to parse the stored data. Every type is expected to be best suited for a specific file system or medium type, or be better performing while used with external systems. In any case, all configured files are automatically synchronized whenever a value changes.

## Cached Saves

It is possible to enable cached saves, via Kconfig, along with the time (in ms) that should elapse after the last storage value was updated before a write to the file or files should be initiated.

## Storage Types
The current storage types are defined in <enum storage_type_e> and are as follows.

### STORAGE_BINARY

Data is stored as "raw" binary. Bool, int and float data are stored using the maximum number of bytes needed, as determined by the size of the union used for these settings; this will be architecture dependent.

### STORAGE_TEXT

All data is converted to ASCII characters making the storage easily human-readable.

# Usage

## Most common

This is the usual sequence of calls to use the settings utility.

1. Call <code>settings_init();</code>
2. Call <code>settings_setstorage("path", storage_type);</code> for every/any file storage in use
  - for example <code>settings_setstorage("/dev/eeprom", STORAGE_EPROM);</code>
3. Call <code>settings_create(key_name, settings_type, default value)</code> for every setting required
4. <code>settings_notify()</code>. 
  - for example <code>settings_create("KEY1", SETTINGS_STRING, "Hello");</code>
5. If a settings value needs to be read, call the variadic function <code>settings_get(key_name, settings_type, &variable, size);</code>. "size" is only needed for STRING types.
  - for example <code>settings_get("KEY1", SETTINGS_STRING, &read_str, sizeof(readstr));</code>
6. If a settings value needs to be changed, call the variadic function <code>settings_set(key_name, settings_type, &value, size);</code> "size" is only needed for STRING types.
  - for example <code>settings_set("KEY1", SETTINGS_STRING, &new_string);</code>

## Other functions
Other functions exist to assist the user program.
1. <code>settings_sync(wait_dump)</code>. This will synchronise the settings between file storages if more than 1 are in use. if cached saves are enabled, the <code>wait_dump</code> parameter, if true, will force the sync function to wait until any saves are actually written out.
2. <code>settings_iterate(idx, &setting)</code>. This gets a copy of a setting at the specified position. It can be used to iterate over the entire settings map, by using successive values of idx.
3. <code>settings_type(key_name, &type)</code>. Gets the type of a given setting.
4. <code>settings_clear()</code>. Clears all settings and sata in all storages is purged.
5. <code>settings_hash(&hash)</code>. Gets the hash of the settings storage. This hash represents the internal state of the settings map. A unique number is calculated based on the contents of the whole map. This hash can be used to check the settings for any alterations: i.e. any setting that may had its value changed since last check.
## Error codes
The settings functions provide negated error return codes that can be used as required by the user application to deal with unexpected behaviour.

## Example App
There is an example app available to demonstrate the usage of many of the settings features <code>CONFIG_EXAMPLES_SETTINGS</code>.
