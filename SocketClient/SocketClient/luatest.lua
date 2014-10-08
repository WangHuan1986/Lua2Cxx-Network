--指定了lua搜索的根目录，所有相对路径都是相对这个目录进行查找的
_G.package.path = _G.package.path .. ";/Users/wanghuan/mine/SocketClient/SocketClient/?.lua"
--[[
   Author: Julio Manuel Fernandez-Diaz
   Date:   January 12, 2007
   (For Lua 5.1)
   
   Modified slightly by RiciLake to avoid the unnecessary table traversal in tablecount()

   Formats tables with cycles recursively to any depth.
   The output is returned as a string.
   References to other tables are shown as values.
   Self references are indicated.

   The string returned is "Lua code", which can be procesed
   (in the case in which indent is composed by spaces or "--").
   Userdata and function keys and values are shown as strings,
   which logically are exactly not equivalent to the original code.

   This routine can serve for pretty formating tables with
   proper indentations, apart from printing them:

      print(table.show(t, "t"))   -- a typical use
   
   Heavily based on "Saving tables with cycles", PIL2, p. 113.

   Arguments:
      t is the table.
      name is the name of the table (optional)
      indent is a first indentation (optional).
--]]
function table.show(t, name, indent)
   local cart     -- a container
   local autoref  -- for self references

   --[[ counts the number of elements in a table
   local function tablecount(t)
      local n = 0
      for _, _ in pairs(t) do n = n+1 end
      return n
   end
   ]]
   -- (RiciLake) returns true if the table is empty
   local function isemptytable(t) return next(t) == nil end

   local function basicSerialize (o)
      local so = tostring(o)
      if type(o) == "function" then
         local info = debug.getinfo(o, "S")
         -- info.name is nil because o is not a calling level
         if info.what == "C" then
            return string.format("%q", so .. ", C function")
         else 
            -- the information is defined through lines
            return string.format("%q", so .. ", defined in (" ..
                info.linedefined .. "-" .. info.lastlinedefined ..
                ")" .. info.source)
         end
      elseif type(o) == "number" or type(o) == "boolean" then
         return so
      else
         return string.format("%q", so)
      end
   end

   local function addtocart (value, name, indent, saved, field)
      indent = indent or ""
      saved = saved or {}
      field = field or name

      cart = cart .. indent .. field

      if type(value) ~= "table" then
         cart = cart .. " = " .. basicSerialize(value) .. ";\n"
      else
         if saved[value] then
            cart = cart .. " = {}; -- " .. saved[value] 
                        .. " (self reference)\n"
            autoref = autoref ..  name .. " = " .. saved[value] .. ";\n"
         else
            saved[value] = name
            --if tablecount(value) == 0 then
            if isemptytable(value) then
               cart = cart .. " = {};\n"
            else
               cart = cart .. " = {\n"
               for k, v in pairs(value) do
                  k = basicSerialize(k)
                  local fname = string.format("%s[%s]", name, k)
                  field = string.format("[%s]", k)
                  -- three spaces between levels
                  addtocart(v, fname, indent .. "   ", saved, field)
               end
               cart = cart .. indent .. "};\n"
            end
         end
      end
   end

   name = name or "__unnamed__"
   if type(t) ~= "table" then
      return name .. " = " .. basicSerialize(t)
   end
   cart, autoref = "", ""
   addtocart(t, name, indent)
   return cart .. autoref
end

---------------------------------------------------------------------------------

g = {}
g["MESSAGE_TYPE"] = {}
g["MESSAGE_TYPE"][1001] = "1001"
g["MESSAGE_TYPE"][1002] = "1002"
g["MESSAGE_TYPE"][1003] = "1003"
g["MESSAGE_TYPE"][1004] = "1004"
g["MESSAGE_TYPE"][1005] = "1005"
g["MESSAGE_TYPE"][1006] = "1006"
g["MESSAGE_TYPE"][1007] = "1007"
g["MESSAGE_TYPE"][1008] = "1008"
g["MESSAGE_TYPE"][1009] = "1009"
g["MESSAGE_TYPE"][1010] = "1010"
g["MESSAGE_TYPE"][1011] = "1011"

-- --tc_1
local data1 = {
   ["messageId"] = g["MESSAGE_TYPE"][1001],
   ["name"] = "F"
}

-- --tc_2
local data2 = {
  ["messageId"] = g["MESSAGE_TYPE"][1002],
  ["o"] = {["name"] = nil}
}

-- --tc_3
local data3 = {
  ["messageId"] = g["MESSAGE_TYPE"][1003],
  ["mylist"] = {
    {["name"] = "D"}
  }
}

-- --tc_4
local data4 = {
  ["messageId"] = g["MESSAGE_TYPE"][1004],
  ["A"] = {
    ["B"] = {
      ["C"] = {
        ["D"] = {
            ["name"] = "wh"
        }
      }
    }
  },
  ["E"] = {}
}

-- --tc_5
local data5 = {
  ["messageId"] = g["MESSAGE_TYPE"][1005],
  ["A"] = {
    ["name"] = "A",
    ["B"] = {
      ["C"] = {
        ["D"] = {
        }
      }
    }
  },
  ["E"] = {}
}

-- --tc_6
local data6 = {
  ["messageId"] = g["MESSAGE_TYPE"][1006],
  ["name"] = "data",
  ["A"] = {},
  ["B"] = {},
  ["C"] = {},
  ["D"] = {
    ["E"] = {
      ["F"] = {}
    }
  }
}

-- --tc_7
local data7 = {
  ["messageId"] = g["MESSAGE_TYPE"][1007],
  ["string_field"] = "wh",
  ["boolean_field"] = true,
  ["char_field"] = 27,
  ["short_field"] = 32767,
  ["int_field"] = 2147483647,
  ["long_field"] = 99999999999999,
  ["double_field"] = 123456789.123456
}

-- -- tc_8
local data8 = {
   ["messageId"] = g["MESSAGE_TYPE"][1008],
   ["string_field"] = "wh",
   ["boolean_field"] = true,
   ["byte_field"] = 27,
   ["short_field"] = 32767,
   ["int_field"] = 2147483647,
   ["long_field"] = 99999999999999,
   ["wife"] = {
        ["wife_name"] = {
          ["big_name"] = "yj",
          ["small_name"] = "jj"
        },
        ["kids"] = {
            {["name"] = "kid1",["age"] = 1},
            {["name"] = "kid2",["age"] = 2},
            {["name"] = "kid3",["age"] = 3}
        }
    }
}

-- --tc_9
local data9 = {
  ["messageId"] = g["MESSAGE_TYPE"][1009],
  ["wife"] = {
    ["father"] = {
      ["dogs"] = {
        {
          ["owners"] = {
            {
              ["name"] = "dog1_owner1",
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog1_owner2",
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            }
          }
        }
      }
    }
  }
}

-- --tc_10
local data10 = {
  ["messageId"] = g["MESSAGE_TYPE"][1010],
  ["name"] = "wh",
  ["wife"] = {
    ["wife_name"] = {
      ["big_name"] = "yj",
      ["small_name"] = "jj"
    },
    ["age"] = 27,
    ["father"] = {
      ["name"] = "father Y",
      ["age"] = 58,
      ["dogs"] = {
        {
          ["name"] = "dog1",
          ["age"] = 1,
          ["owners"] = {
            {
              ["name"] = "dog1_owner1",
              ["age"] = 1,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog1_owner2",
              ["age"] = 2,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog1_owner3",
              ["age"] = 3,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            }
          }
        },
        {
          ["name"] = "dog2",
          ["age"] = 2,
          ["owners"] = {
            {
              ["name"] = "dog2_owner1",
              ["age"] = 1,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog2_owner2",
              ["age"] = 2,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog2_owner3",
              ["age"] = 3,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            }
          }
        },
        {
          ["name"] = "dog3",
          ["age"] = 3,
          ["owners"] = {
            {
              ["name"] = "dog3_owner1",
              ["age"] = 1,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog3_owner2",
              ["age"] = 2,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            },
            {
              ["name"] = "dog3_owner3",
              ["age"] = 3,
              ["farms"] = {
                {["name"] = "farm1"},
                {["name"] = "farm2"},
                {["name"] = "farm3"}
              }
            }
          }
        }
      }
    }
  },
  ["kids"] = {
    {["name"] = "kid1",["age"] = 1},
    {["name"] = "kid2",["age"] = 2},
    {["name"] = "kid3",["age"] = 3}
  }
}

local data11 = {
  ["messageId"] = g["MESSAGE_TYPE"][1011],
  ["wifes"] = {
    {["name"] = "w1"},
    {["name"] = "w2"}
  },
  ["string_field"] = "wh"
}



print(table.show(data11,"data"))

function helloWorld ()
  print(c_myLib.sendData(data11))
end

helloWorld()

function luaCallBack( t )
  -- body
  print("========== result =========")
  print(table.show(t,"data"))
end

