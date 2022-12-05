---
title: 'Cuneicode: An Encoding Conversion Library for Unicode and Beyond'
tags:
  - C
  - C++
  - systems
  - text
  - encoding
authors:
  - name: Björkus Dorkus
    orcid: 0000-0003-3385-2624
    corresponding: true
    affiliation: "1, 2"
  - name: Joël Falcou
    affiliation: 1
affiliations:
  - name: Univerisité Paris Saclay
    index: 1
  - name: Shepherd's Oasis, LLC
    index: 2
date: 15 July 2022
bibliography: paper.bib
---



# Summary

We present a C interface to solve both two specific problems endemic to systems programmers. The first is to convert text data that is locale (native, typically pre-installed, operating system-determined) encoding to Unicode, and back, without loss of information. The second is to convert text data from any source to any other arbitrary encoding without needing to specifically program a routine to go from one disparate encoding to another disparate encoding (e.g., the Shift Japanese Industrial Standard X 0208 (Shift JIS X 0208) encoding to the Big-5 encoding). We describe the direct conversions and why they are specifically named, historically necessary, and superior in design for the ISO/IEC 9899 Programming Languages C (or simply "C") approach to text encodings, and then describe the general-purpose text encoding "Two-Step" that allows us to convert from any single arbitrary encoding to another arbitrary encoding, providing they follow rudimentary rules for how they define their conversions.




# Statement of need

Support for moving from legacy C encodings to Unicode is deeply lacking in the C and C++ systems programming languages. While UTF-8 support for websites has climbed higher than 97% of all websites and pages by 2022 [@w3techs-survey], passports for the authors of this paper in France remove the accent mark and mangle the names, while German taxpayers cannot purchase land or properly pay taxes if their name contains an umlaut (and must call in to specifically fix this with their government agency) [@bangbangcon-unicode]. The vast gulf between Unicode support on the forms and services used online and the forms and services running our medical infrastructure, government websites, and critical infrastructure has created no shortage of data issues with real-world impact.

Many libraries have been brought to solve this problem. From the International Components of Unicode (ICU) [@icu], the C Standard Library [§7.29 and §7.30 @iso-c] and POSIX-conforming iconv [@iconv], to the C Standard's conversion facilities and bespoke systems programming libraries open-sourced throughout the years like encoding_rs [@encoding_rs], utf8-cpp [@utf8-cpp]




# Concrete Conversion Interfaces




# Acknowledgements

We acknowledge Joseph Myers, Dr. Philip K. Krause, Robert C. Seacord, and Aaron Ballman for significant help in improving the C interface used as the basis for this project. We thank Henri Sivonen, the Unicode Consortium, and the GNU libiconv project for their significant contributions to the area of text encoding. We would also like to thank Tom Honermann, Steve Downey, Corentin Jabot, and others for improvements to the area of C and C++ for text encoding.




# References
