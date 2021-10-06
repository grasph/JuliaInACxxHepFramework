module EventLoopGlue

export rejected, passed, Event, JlAnalysis

using CxxWrap
@wrapmodule("/home/pgras/dev/julia/julia_from_c/EventLoopGlue.so")

function __init__()
    @initcxx
end

rejected = false
passed = true

function analyze(event::Ptr{Event})::Cint
    return one(Int32)
end

abstract type JlAnalysis end

function print_stack()
    println(stderr, "In print_stack")
    for (exc, bt) in Base.catch_stack()
        showerror(stderr, exc, bt)
        println(stderr)
    end
    println("---")
end

end #module
