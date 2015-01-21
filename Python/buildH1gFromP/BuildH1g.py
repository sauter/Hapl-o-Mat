#Jan 2015
#Christian Schaefer
#Create list of g-codes by combining lists of G and P codes. If all codes in a P-line can be found in a G-line, add codes ending with a letter to the P-line. Write results to H1g.txt

#save each line of P.txt as P-code plus a set of the corresponding codes
allPCodes = []
with open('P.txt') as file:
    for line in file:
        line = line.rstrip('\n')
        splittedLine = line.split()
        pCode = []
        pCode.append(splittedLine[0])

        splittedLine.pop(0)
        codes = set()
        codes.update(splittedLine)
        pCode.append(codes)
        
        allPCodes.append(pCode)

#
endLetters = ('N', 'L', 'S', 'Q')
with open('H1.txt') as file:
    for line in file:
        line = line.rstrip('\n')
        splittedLine = line.split()
        #build two sets, one containing codes with letters at the end, the other one the remaining codes
        splittedLine.pop(0)
        codesN = set()
        codes = set()
        for code in splittedLine:
            if code.endswith(endLetters):
                codesN.add(code)
            else:
                codes.add(code)

        for pCode in allPCodes:
            if codes.issubset(pCode[1]) and codes:
                pCode[1].update(codesN)
        
with open('H1g.txt', 'w') as file:
    for pCode in allPCodes:

        gCode = pCode[0][:-1]
        sameTwoDigit = True

        shorterCodes = set()
        for code in pCode[1]:
            shorterCodes.add(code)    
            if code.count(':') > 1:
                shorterCode = code.rsplit(':',1)[0]
                shorterCodes.add(shorterCode)

            splittedCode = code.split(':')
            TwoDigitCode = splittedCode[0] + ':' + splittedCode[1]
            if TwoDigitCode == gCode:
                sameTwoDigit = sameTwoDigit and True
            else:
                sameTwoDigit = False
        
        if not sameTwoDigit:
            gCode += 'g'
            
        sortedShorterCodes = sorted(shorterCodes)
        file.write(gCode + '\t' + '\t'.join(sortedShorterCodes) + '\n')