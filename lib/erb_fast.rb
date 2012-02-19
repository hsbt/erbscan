require "erb"
require "erbscan"

class ERB
  class Compiler
    def initialize(trim_mode)
      @percent, @explicit_trim, @trim_mode = prepare_trim_mode(trim_mode)
      @put_cmd = 'print'
      @pre_cmd = []
      @post_cmd = []
    end
    
    def prepare_trim_mode(mode)
      perc = false
      explicit_trim = false
      trim_mode = nil
      case mode
      when Fixnum
        trim_mode = case mode
          when 1
            '>'
          when 2
            '<>'
          else
            nil
          end
      when String
        perc = mode.include?('%')
        explicit_trim = mode.include?('-')
        if mode.include?('<>')
          trim_mode = '<>'
        elsif mode.include?('>')
          trim_mode = '>'
        end
      end
      [perc, explicit_trim, trim_mode]
    end
    
    def compile(s)
      @__cmd = ''
      @__cmd << @pre_cmd.join('') << "\n"
      
      scanner = ERBScanner.new
      case @trim_mode
      when '<'
        scanner.trim_mode = 1
      when '<>'
        scanner.trim_mode = 2
      else
        scanner.trim_mode = 0
      end
      scanner.percent       = @percent ? 1 : 0
      scanner.explicit_trim = @explicit_trim ? 1 : 0
      scanner.scan(self, s)
      
      @__cmd << @post_cmd.join('') << "\n"
      
      @__cmd
    end
    
    def text(str)
      @__cmd << "#{@put_cmd} #{str.dump};\n"
    end
    
    def code(str)
      @__cmd << str << "\n"
    end
    
    def code_percent(str)
      @__cmd << str << "\n"
    end
    
    def code_put(str)
      @__cmd << "#{@put_cmd}((#{str}).to_s);\n"
    end
    
    def code_comment(str)
    end
  end
end

