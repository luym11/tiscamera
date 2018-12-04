/*
 * Copyright 2018 The Imaging Source Europe GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "uvc-extension-loader.h"

#include "utils.h"

#include "json.hpp"

#include <sys/ioctl.h>

#include <uuid/uuid.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <limits.h> // LONG_MAX

#include <fstream>

using json = nlohmann::json;


int tcam::uvc::map (int fd,
                    uvc_xu_control_mapping* ctrl)
{
    return tcam_xioctl(fd, UVCIOC_CTRL_MAP, ctrl);
}


void tcam::uvc::apply_mappings (int fd,
                                std::vector<tcam::uvc::description>& mappings,
                                std::function<void(const std::string&)> cb)
{
    for (auto& m : mappings)
    {
        if (m.mapping.v4l2_type == V4L2_CTRL_TYPE_MENU)
        {
            m.mapping.menu_info = m.entries.data();
            m.mapping.menu_count = m.entries.size();
        }

        int ret = map(fd, &m.mapping);

        if (ret != 0)
        {
            std::string msg = "Error while mapping '" + std::string((char*)m.mapping.name)
                + "': errno: " + std::to_string(errno) + " - " + strerror(errno);
            cb(msg);
        }
    }
}


__u8 string_to_u8 (const std::string& input)
{
    char* p;
    long n = strtol( input.c_str(), & p, 16 );

    // this means an error occured
    if (n == LONG_MAX || n == LONG_MIN)
    {
        return 0;
    }

    return n;
}


__u8 parse_uvc_type (const std::string& str)
{
    if (str == "unsigned")
    {
        return UVC_CTRL_DATA_TYPE_UNSIGNED;
    }
    else if (str == "signed")
    {
        return UVC_CTRL_DATA_TYPE_SIGNED;
    }
    else if (str == "raw")
    {
        return UVC_CTRL_DATA_TYPE_RAW;
    }
    else if (str == "enum")
    {
        return UVC_CTRL_DATA_TYPE_ENUM;
    }
    else if (str == "boolean")
    {
        return UVC_CTRL_DATA_TYPE_BOOLEAN;
    }
    else if (str == "bitmask")
    {
        return UVC_CTRL_DATA_TYPE_BITMASK;
    }

    return 0;
}


__u8 parse_v4l2_type (const std::string& str)
{
    if (str == "bitmask")
    {
        return V4L2_CTRL_TYPE_BITMASK;
    }
    else if (str == "boolean")
    {
        return V4L2_CTRL_TYPE_BOOLEAN;
    }
    else if (str == "button")
    {
        return V4L2_CTRL_TYPE_BUTTON;
    }
    else if (str == "integer")
    {
        return V4L2_CTRL_TYPE_INTEGER;
    }
    else if (str == "menu")
    {
        return V4L2_CTRL_TYPE_MENU;
    }
    else if (str == "string")
    {
        return V4L2_CTRL_TYPE_STRING;
    }

    return 0;
}


std::vector<tcam::uvc::description> tcam::uvc::load_description_file (const std::string& filename,
                                                                      std::function<void(const std::string&)> cb)
{
    static const int max_string_length = 31;

    uuid_t guid;

    std::vector<description> mappings;

    std::ifstream ifs(filename);

    json json_desc;

    try
    {
        ifs >> json_desc;
    }
    catch (json::parse_error& err)
    {
        std::string msg = std::string(err.what())
            + " - This occurred around byte " + std::to_string(err.byte);
        cb(msg);
        return mappings;
    }

    uuid_parse(json_desc.at("guid").get<std::string>().c_str(), guid);

    for (const auto& m : json_desc.at("mappings"))
    {
        struct description desc;
        desc.mapping = {};

        auto& map = desc.mapping;

        map.id = string_to_u8(m.at("id").get<std::string>());

        if (map.id == 0)
        {
            std::string msg = "Could not convert id field to valid u8 for "
                + m.at("name").get<std::string>();
            cb(msg);
            continue;
        }

        if (m.at("name").get<std::string>().size() > max_string_length)
        {
            std::string msg = "V4L2 name is to long! " + m.at("name").get<std::string>()
                + " will be cut off. Reduce to 31 Characters or less.";
            cb(msg);
        }
        strcpy((char*)map.name, m.at("name").get<std::string>().c_str());

        map.selector = string_to_u8(m.at("selector").get<std::string>());

        map.size = m.at("size_bits").get<int>();
        map.offset = m.at("offset_bits").get<int>();

        map.data_type = parse_uvc_type(m.at("uvc_type").get<std::string>());

        if (map.data_type == 0)
        {
            std::string msg = "data_type for '" + m.at("name").get<std::string>()
                + "' does not make sense.";
            cb(msg);
            continue;
        }

        map.v4l2_type = parse_v4l2_type(m.at("v4l2_type").get<std::string>());
        if (map.data_type == 0)
        {
            std::string msg = "v4l2_type for '" + m.at("name").get<std::string>()
                + "' does not make sense.";
            cb(msg);
            continue;
        }

        memcpy(map.entity, guid, sizeof(map.entity));

        if (map.v4l2_type == V4L2_CTRL_TYPE_MENU)
        {
            for (auto& e : m.at("entries"))
            {
                uvc_menu_info entry = {};

                entry.value = e.at("value").get<int>();
                // TODO length check

                std::string str = e.at("entry").get<std::string>();

                if (str.size() > max_string_length)
                {
                    std::string msg = "Menu Entry '" + str
                        + "' is too long. Reduce to 31 Characters or less.";
                    cb(msg);
                }

                strcpy((char*)entry.name, str.c_str());

                desc.entries.push_back(entry);
            }

        }
        mappings.push_back(desc);
    }

    return mappings;
}
