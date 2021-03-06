module FSM 
    ( ASym (Symbol, Epsilon),
      MState (State),
      FSMTransition (Transition, from, input, to),
      FSMachine (Machine, states, alphabet, transitions, sstate, fstates),
      inferStates,
      buildFSMachine ) where

data ASym a = Symbol a | Epsilon
    deriving (Show,Ord,Eq,Read)

data MState a = State a
    deriving (Show,Ord,Eq,Read)

data FSMTransition = Transition {
        from  :: MState [Char],
        input :: ASym [Char],
        to    :: MState [Char] }
    deriving (Show,Ord,Eq,Read)

data FSMachine = Machine {
        states      :: [MState [Char]],
        alphabet    :: [ASym [Char]],
        transitions :: [FSMTransition],
        sstate      :: MState [Char],
        fstates     :: [MState [Char]] }
    deriving (Show,Ord,Eq,Read)

__inferAlphabet :: [FSMTransition] -> [ASym [Char]]
__inferAlphabet [] = []
__inferAlphabet ((Transition{input=i}):[]) = i:[]
__inferAlphabet ((Transition{input=i}):ts) = i:(__inferAlphabet ts)

_rmDup :: Eq a => [a] -> [a]
_rmDup [] = []
_rmDup (a:[]) = a:[]
_rmDup (a:as) | a `elem` as = _rmDup as
              | otherwise   = a:(_rmDup as)

_inferAlphabet :: [FSMTransition] -> [ASym [Char]]
_inferAlphabet ts = filter (/= Epsilon) (_rmDup (__inferAlphabet ts))

_inferStates :: [FSMTransition] -> [MState [Char]]
_inferStates [] = []
_inferStates ((Transition{from=f,to=t}):[]) | f == t    = f:[]
                                             | otherwise = f:t:[]
_inferStates ((Transition{from=f,to=t}):ts) | f == t    = f:(_inferStates ts)
                                             | otherwise = f:t:(_inferStates ts)

inferStates :: [FSMTransition] -> [MState [Char]]
inferStates ts = _rmDup (_inferStates ts)

-- The alphabet of the machine and its states are inferred from the transitions 
-- supplied to the build function. The start state and every final state should
-- be from the inferred states set.
buildFSMachine :: [FSMTransition] -> MState [Char] -> [MState [Char]] -> Maybe FSMachine
buildFSMachine ts ss fs
    | (ss `elem` inferredStates) && (all (`elem` inferredStates) fs) = Just Machine {
        states      = inferredStates,
        alphabet    = inferredAlphabet,
        transitions = ts,
        sstate      = ss,
        fstates     = fs }
    | otherwise = Nothing
    where
        inferredStates   = inferStates ts
        inferredAlphabet = _inferAlphabet ts
