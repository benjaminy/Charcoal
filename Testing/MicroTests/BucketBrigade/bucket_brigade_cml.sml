
structure Main = struct
  val N = 10
  val M = 10000000

  fun f (my_chan, next_chan) =
    let
      fun loop n =
        if n < 1 then ()
        else
          let val () = CML.recv my_chan
              val () = CML.send (next_chan, ())
          in
              loop (n-1)
          end
    in
      loop M
    end

  val x = CML.version

  fun main () =
    let
      val chans = Array.tabulate(N, fn _ => CML.channel())
      fun s(i, chan, ts) =
        let val t = CML.spawnc f (Array.sub(chans, i), Array.sub(chans, (i + 1) mod N)) in
        t::ts
        end
      val threads = Array.foldli s [] chans
      val () = CML.send (Array.sub(chans, 0), ())
    in
      ()
    end

  fun m() = RunCML.doit(main, NONE)
end
