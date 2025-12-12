# Tool calling smoke-test for mkxp-z's Ollama Ruby binding.
#
# Requires:
# - Ollama running on 127.0.0.1:11434
# - A model with tool calling support (set ENV["OLLAMA_MODEL"], default: "ministral-3:8b")
#
# Run via mkxp.json "customScript".

model = ENV["OLLAMA_MODEL"] || "ministral-3:8b"

tools = [
  {
    "type" => "function",
    "function" => {
      "name" => "get_temperature",
      "description" => "Get the current temperature for a city",
      "parameters" => {
        "type" => "object",
        "required" => ["city"],
        "properties" => {
          "city" => { "type" => "string", "description" => "The name of the city" }
        }
      }
    }
  },
  {
    "type" => "function",
    "function" => {
      "name" => "get_conditions",
      "description" => "Get the current weather conditions for a city",
      "parameters" => {
        "type" => "object",
        "required" => ["city"],
        "properties" => {
          "city" => { "type" => "string", "description" => "The name of the city" }
        }
      }
    }
  }
]

messages = [
  {
    "role" => "user",
    "content" => "What are the current weather conditions and temperature in New York and London?"
  }
]

done = false
error = false
first = nil

System::puts "\nOllama tool calling (phase 1)"
Ollama.chat({ "model" => model, "messages" => messages, "tools" => tools }) do |resp, err|
  first = resp
  error = err
  done = true
end

while !done
  if defined?(Graphics) && Graphics.respond_to?(:wait)
    Graphics.wait(1)
  else
    sleep(0.01)
  end
end

if error
  System::puts "Request failed:\n#{first.inspect}"
  exit
end

tool_calls = (first["message"] && first["message"]["tool_calls"]) || []
if tool_calls.empty?
  System::puts "No tool_calls returned:\n#{first.inspect}"
  exit
end

System::puts "Tool calls:\n#{tool_calls.inspect}"

assistant_msg = { "role" => "assistant", "tool_calls" => tool_calls }

tool_messages = []
tool_calls.each do |call|
  fn = call["function"] || {}
  name = fn["name"].to_s
  args = fn["arguments"] || {}
  city = args["city"].to_s
  call_id = call["id"]

  result =
    case name
    when "get_temperature"
      case city
      when "New York" then "22°C"
      when "London" then "15°C"
      else "0°C"
      end
    when "get_conditions"
      case city
      when "New York" then "Partly cloudy"
      when "London" then "Rainy"
      else "Unknown"
      end
    else
      "Unknown tool: #{name}"
    end

  tool_msg = { "role" => "tool", "tool_name" => name, "content" => result }
  tool_msg["tool_call_id"] = call_id if call_id
  tool_messages << tool_msg
end

messages2 = messages + [assistant_msg] + tool_messages

done2 = false
error2 = false
second = nil

System::puts "\nOllama tool calling (phase 2)"
Ollama.chat({ "model" => model, "messages" => messages2 }) do |resp, err|
  second = resp
  error2 = err
  done2 = true
end

while !done2
  if defined?(Graphics) && Graphics.respond_to?(:wait)
    Graphics.wait(1)
  else
    sleep(0.01)
  end
end

if error2
  System::puts "Follow-up request failed:\n#{second.inspect}"
  exit
end

System::puts "Final response:\n#{second.inspect}"
System::gets
exit
