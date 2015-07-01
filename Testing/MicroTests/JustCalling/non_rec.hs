

f01 :: Int -> Int
f01 a = (a * 17 + 29) `mod` 43

f02 :: Int -> Int
f02 a = a `seq` b `seq` f01 b
  where
    b = f01 a
f03 :: Int -> Int
f03 a = a `seq` b `seq` f02 b
  where
    b = f02 a
f04 :: Int -> Int
f04 a = a `seq` b `seq` f03 b
  where
    b = f03 a
f05 :: Int -> Int
f05 a = a `seq` b `seq` f04 b
  where
    b = f04 a
f06 :: Int -> Int
f06 a = a `seq` b `seq` f05 b
  where
    b = f05 a
f07 :: Int -> Int
f07 a = a `seq` b `seq` f06 b
  where
    b = f06 a
f08 :: Int -> Int
f08 a = a `seq` b `seq` f07 b
  where
    b = f07 a
f09 :: Int -> Int
f09 a = a `seq` b `seq` f08 b
  where
    b = f08 a
f10 :: Int -> Int
f10 a = a `seq` b `seq` f09 b
  where
    b = f09 a
f11 :: Int -> Int
f11 a = a `seq` b `seq` f10 b
  where
    b = f10 a
f12 :: Int -> Int
f12 a = a `seq` b `seq` f11 b
  where
    b = f11 a
f13 :: Int -> Int
f13 a = a `seq` b `seq` f12 b
  where
    b = f12 a
f14 :: Int -> Int
f14 a = a `seq` b `seq` f13 b
  where
    b = f13 a
f15 :: Int -> Int
f15 a = a `seq` b `seq` f14 b
  where
    b = f14 a
f16 :: Int -> Int
f16 a = a `seq` b `seq` f15 b
  where
    b = f15 a
f17 :: Int -> Int
f17 a = a `seq` b `seq` f16 b
  where
    b = f16 a
f18 :: Int -> Int
f18 a = a `seq` b `seq` f17 b
  where
    b = f17 a
f19 :: Int -> Int
f19 a = a `seq` b `seq` f18 b
  where
    b = f18 a
f20 :: Int -> Int
f20 a = a `seq` b `seq` f19 b
  where
    b = f19 a
f21 :: Int -> Int
f21 a = a `seq` b `seq` f20 b
  where
    b = f20 a
f22 :: Int -> Int
f22 a = a `seq` b `seq` f21 b
  where
    b = f21 a
f23 :: Int -> Int
f23 a = a `seq` b `seq` f22 b
  where
    b = f22 a
f24 :: Int -> Int
f24 a = a `seq` b `seq` f23 b
  where
    b = f23 a
f25 :: Int -> Int
f25 a = a `seq` b `seq` f24 b
  where
    b = f24 a
f26 :: Int -> Int
f26 a = a `seq` b `seq` f25 b
  where
    b = f25 a
f27 :: Int -> Int
f27 a = a `seq` b `seq` f26 b
  where
    b = f26 a
f28 :: Int -> Int
f28 a = a `seq` b `seq` f27 b
  where
    b = f27 a
f29 :: Int -> Int
f29 a = a `seq` b `seq` f28 b
  where
    b = f28 a
f30 :: Int -> Int
f30 a = a `seq` b `seq` f29 b
  where
    b = f29 a
f31 :: Int -> Int
f31 a = a `seq` b `seq` f30 b
  where
    b = f30 a
f32 :: Int -> Int
f32 a = a `seq` b `seq` f31 b
  where
    b = f31 a
f33 :: Int -> Int
f33 a = a `seq` b `seq` f32 b
  where
    b = f32 a
f34 :: Int -> Int
f34 a = a `seq` b `seq` f33 b
  where
    b = f33 a
f35 :: Int -> Int
f35 a = a `seq` b `seq` f34 b
  where
    b = f34 a
f36 :: Int -> Int
f36 a = a `seq` b `seq` f35 b
  where
    b = f35 a
f37 :: Int -> Int
f37 a = a `seq` b `seq` f36 b
  where
    b = f36 a
f38 :: Int -> Int
f38 a = a `seq` b `seq` f37 b
  where
    b = f37 a
f39 :: Int -> Int
f39 a = a `seq` b `seq` f38 b
  where
    b = f38 a
f40 :: Int -> Int
f40 a = a `seq` b `seq` f39 b
  where
    b = f39 a

main :: IO ()
main = do
  putStrLn( show ( f30 3 ) )
