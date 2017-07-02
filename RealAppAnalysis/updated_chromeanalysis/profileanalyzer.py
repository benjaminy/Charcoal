import parseprofile as parser

def main():
    profile = parser.load_profile_from_file("profiles/sample.json")
    categorized_events = parser.parseprofile( profile  )
    print process_with_most_durationevents(categorized_events)
    print thread_with_most_durationevents(categorized_events)


def process_with_most_durationevents(categorized_events):
    process_of_interst = 0
    max_number_of_durationevents = -1

    for pid, subdic in categorized_events.items():
        number_of_durationevents = 0
        for tid, subsubdic in subdic.items():
            if 'durationevents' in subsubdic:
                number_of_durationevents += len(subsubdic['durationevents'])
        if( number_of_durationevents > max_number_of_durationevents ):
            max_number_of_durationevents = number_of_durationevents
            process_of_interst = pid

    return process_of_interst


def thread_with_most_durationevents(categorized_events):
    thread_of_interst = (0, 0)
    max_number_of_durationevents = -1

    for pid, subdic in categorized_events.items():
        for tid, subsubdic in subdic.items():
            number_of_durationevents = 0
            if 'durationevents' in subsubdic:
                number_of_durationevents += len(subsubdic['durationevents'])
        if( number_of_durationevents > max_number_of_durationevents ):
            max_number_of_durationevents = number_of_durationevents
            thread_of_interst = (pid, tid)

    return thread_of_interst


if __name__ == '__main__':
    main()
