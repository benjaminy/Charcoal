import profileparser
import dataanalysis
import os


def main():
    for profile in os.listdir("profiles"):
        profileparser.generate_csv("profiles/" + profile)

    for csvfile in os.listdir("csvfiles"):
        dataanalysis.generate_graph("csvfiles/" + csvfile)


if __name__ == '__main__':
    main()
