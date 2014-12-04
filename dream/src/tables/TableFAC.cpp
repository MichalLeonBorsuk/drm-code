/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Tables for FAC
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "TableFAC.h"

/* Definitions ****************************************************************/
/* ETSI ES201980 V4.1.1: page 89, 7.2:
 * ...FAC shall use 4-QAM mapping. A
   fixed code rate shall be applied...R_all=0.6...
   6 tailbits are used for the encoder to get in zero state ->
   65 [number of cells] * 2 [4-QAM] * 0.6 [code-rate] - 6 [tailbits] = 72

The number of bits
L
FAC
per FAC block equals 72 bits in robustness modes A, B, C and D and 116 bits in robustness
mode E.

*/
const int iTableNumOfFACbitsPerBlock[] = {72,72,72,72,116}; // A=0 ... E=4

/* iTableNumOfServices[a][b]
   a: Number of audio services
   b: Number of data services
   (6.3.4) */
const int iTableNumOfServices[5][5] = {
    /* -> Data */
    {-1,  1,  2,  3, 15},
    { 4,  5,  6,  7, -1},
    { 8,  9, 10, -1, -1},
    {12, 13, -1, -1, -1},
    { 0, -1, -1, -1, -1}
};

/* Language code */
#define LEN_TABLE_LANGUAGE_CODE			16

const string strTableLanguageCode[LEN_TABLE_LANGUAGE_CODE] = {
    "No language specified",
    "Arabic",
    "Bengali",
    "Chinese (Mandarin)",
    "Dutch",
    "English",
    "French",
    "German",
    "Hindi",
    "Japanese",
    "Javanese",
    "Korean",
    "Portuguese",
    "Russian",
    "Spanish",
    "Other language"
};

/* Programme Type codes */
#define LEN_TABLE_PROG_TYPE_CODE_TOT	32
#define LEN_TABLE_PROG_TYPE_CODE		30

const string strTableProgTypCod[LEN_TABLE_PROG_TYPE_CODE_TOT] = {
    "No programme type",
    "News",
    "Current Affairs",
    "Information",
    "Sport",
    "Education",
    "Drama",
    "Culture",
    "Science",
    "Varied",
    "Pop Music",
    "Rock Music",
    "Easy Listening Music",
    "Light Classical",
    "Serious Classical",
    "Other Music",
    "Weather/meteorology",
    "Finance/Business",
    "Children's programmes",
    "Social Affairs",
    "Religion",
    "Phone In",
    "Travel",
    "Leisure",
    "Jazz Music",
    "Country Music",
    "National Music",
    "Oldies Music",
    "Folk Music",
    "Documentary",
    "Not used",
    "Not used"
};


/* Country code table according to ISO 3166 */

const struct elCountry TableCountryCode[LEN_TABLE_COUNTRY_CODE] = {
    {"af", "Afghanistan"},
    {"ax", "Aland Islands"},
    {"al", "Albania"},
    {"dz", "Algeria"},
    {"as", "American Samoa"},
    {"ad", "Andorra"},
    {"ao", "Angola"},
    {"ai", "Anguilla"},
    {"aq", "Antarctica"},
    {"ag", "Antigua and barbuda"},
    {"ar", "Argentina"},
    {"am", "Armenia"},
    {"aw", "Aruba"},
    {"au", "Australia"},
    {"at", "Austria"},
    {"az", "Azerbaijan"},
    {"bs", "Bahamas"},
    {"bh", "Bahrain"},
    {"bd", "Bangladesh"},
    {"bb", "Barbados"},
    {"by", "Belarus"},
    {"be", "Belgium"},
    {"bz", "Belize"},
    {"bj", "Benin"},
    {"bm", "Bermuda"},
    {"bt", "Bhutan"},
    {"bo", "Bolivia"},
    {"ba", "Bosnia and Herzegovina"},
    {"bw", "Botswana"},
    {"bv", "Bouvet Island"},
    {"br", "Brazil"},
    {"io", "British Indian Ocean Ter."},
    {"bn", "Brunei Darussalam"},
    {"bg", "Bulgaria"},
    {"bf", "Burkina Faso"},
    {"bi", "Burundi"},
    {"kh", "Cambodia"},
    {"cm", "Cameroon"},
    {"ca", "Canada"},
    {"cv", "Cape Verde"},
    {"ky", "Cayman Islands"},
    {"cf", "Central African Republic"},
    {"td", "Chad"},
    {"cl", "Chile"},
    {"cn", "China"},
    {"cx", "Christmas Island"},
    {"cc", "Cocos (Keeling) Islands"},
    {"co", "Colombia"},
    {"km", "Comoros"},
    {"cg", "Congo Democratic Rep."},
    {"cd", "Congo"},
    {"ck", "Cook Islands"},
    {"cr", "Costa Rica"},
    {"ci", "Côte d'Ivoire"},
    {"hr", "Croatia"},
    {"cu", "Cuba"},
    {"cy", "Cyprus"},
    {"cz", "Czech Republic"},
    {"dk", "Denmark"},
    {"dj", "Djibouti"},
    {"dm", "Dominica"},
    {"do", "Dominican Republic"},
    {"ec", "Ecuador"},
    {"eg", "Egypt"},
    {"sv", "El Salvador"},
    {"gq", "Equatorial Guinea"},
    {"er", "Eritrea"},
    {"ee", "Estonia"},
    {"et", "Ethiopia"},
    {"fk", "Falkland Islands"},
    {"fo", "Faroe Islands"},
    {"fj", "Fiji"},
    {"fi", "Finland"},
    {"fr", "France"},
    {"gf", "French Guiana"},
    {"pf", "French Polynesia"},
    {"tf", "French Southern Ter."},
    {"ga", "Gabon"},
    {"gm", "Gambia"},
    {"ge", "Georgia"},
    {"de", "Germany"},
    {"gh", "Ghana"},
    {"gi", "Gibraltar"},
    {"gr", "Greece"},
    {"gl", "Greenland"},
    {"gd", "Grenada"},
    {"gp", "Guadeloupe"},
    {"gu", "Guam"},
    {"gt", "Guatemala"},
    {"gg", "Guernsey"},
    {"gn", "Guinea"},
    {"gw", "Guinea-Bissau"},
    {"gy", "Guyana"},
    {"ht", "Haiti"},
    {"hm", "Heard Is. Mcdonald Is."},
    {"va", "Vatican City State"},
    {"hn", "Honduras"},
    {"hk", "Hong Kong"},
    {"hu", "Hungary"},
    {"is", "Iceland"},
    {"in", "India"},
    {"id", "Indonesia"},
    {"ir", "Iran"},
    {"iq", "Iraq"},
    {"im", "Isle of Man"},
    {"ie", "Ireland"},
    {"il", "Israel"},
    {"it", "Italy"},
    {"jm", "Jamaica"},
    {"jp", "Japan"},
    {"je", "Jersey"},
    {"jo", "Jordan"},
    {"kz", "Kazakhstan"},
    {"ke", "Kenya"},
    {"ki", "Kiribati"},
    {"kp", "Korea Democratic Rep."},
    {"kr", "Korea, Republic of"},
    {"kw", "Kuwait"},
    {"kg", "Kyrgyzstan"},
    {"la", "Lao People's Democratic Rep."},
    {"lv", "Latvia"},
    {"lb", "Lebanon"},
    {"ls", "Lesotho"},
    {"lr", "Liberia"},
    {"ly", "Libyan Arab Jamahiriya"},
    {"li", "Liechtenstein"},
    {"lt", "Lithuania"},
    {"lu", "Luxembourg"},
    {"mo", "Macao"},
    {"mk", "Macedonia"},
    {"mg", "Madagascar"},
    {"mw", "Malawi"},
    {"my", "Malaysia"},
    {"mv", "Maldives"},
    {"ml", "Mali"},
    {"mt", "Malta"},
    {"mh", "Marshall Islands"},
    {"mq", "Martinique"},
    {"mr", "Mauritania"},
    {"mu", "Mauritius"},
    {"yt", "Mayotte"},
    {"mx", "Mexico"},
    {"fm", "Micronesia"},
    {"md", "Moldova"},
    {"mc", "Monaco"},
    {"mn", "Mongolia"},
    {"me", "Montenegro"},
    {"ms", "Montserrat"},
    {"ma", "Morocco"},
    {"mz", "Mozambique"},
    {"mm", "Myanmar"},
    {"na", "Namibia"},
    {"nr", "Nauru"},
    {"np", "Nepal"},
    {"nl", "Netherlands"},
    {"an", "Netherlands Antilles"},
    {"nc", "New Caledonia"},
    {"nz", "New Zealand"},
    {"ni", "Nicaragua"},
    {"ne", "Niger"},
    {"ng", "Nigeria"},
    {"nu", "Niue"},
    {"nf", "Norfolk Island"},
    {"mp", "Northern Mariana Is."},
    {"no", "Norway"},
    {"om", "Oman"},
    {"pk", "Pakistan"},
    {"pw", "Palau"},
    {"ps", "Palestinian Territory"},
    {"pa", "Panama"},
    {"pg", "Papua New Guinea"},
    {"py", "Paraguay"},
    {"pe", "Peru"},
    {"ph", "Philippines"},
    {"pn", "Pitcairn"},
    {"pl", "Poland"},
    {"pt", "Portugal"},
    {"pr", "Puerto Rico"},
    {"qa", "Qatar"},
    {"re", "Réunion"},
    {"ro", "Romania"},
    {"ru", "Russian Federation"},
    {"rw", "Rwanda"},
    {"sh", "Saint Helena"},
    {"kn", "Saint Kitts and Nevis"},
    {"lc", "Saint Lucia"},
    {"pm", "Saint Pierre and Miquelon"},
    {"vc", "Saint Vincent and the Grenadines"},
    {"ws", "Samoa"},
    {"sm", "San Marino"},
    {"st", "Sao Tome and Principe"},
    {"sa", "Saudi arabia"},
    {"sn", "Senegal"},
    {"rs", "Serbia"},
    {"sc", "Seychelles"},
    {"sl", "Sierra Leone"},
    {"sg", "Singapore"},
    {"sk", "Slovakia"},
    {"si", "Slovenia"},
    {"sb", "Solomon Islands"},
    {"so", "Somalia"},
    {"za", "South Africa"},
    {"gs", "South Georgia South Sandwich Is."},
    {"es", "Spain"},
    {"lk", "Sri Lanka"},
    {"sd", "Sudan"},
    {"sr", "Suriname"},
    {"sj", "Svalbard and Jan Mayen"},
    {"sz", "Swaziland"},
    {"se", "Sweden"},
    {"ch", "Switzerland"},
    {"sy", "Syrian Arab Republic"},
    {"tw", "Taiwan"},
    {"tj", "Tajikistan"},
    {"tz", "Tanzania"},
    {"th", "Thailand"},
    {"tl", "Timor-Leste"},
    {"tg", "Togo"},
    {"tk", "Tokelau"},
    {"to", "Tonga"},
    {"tt", "Trinidad and Tobago"},
    {"tn", "Tunisia"},
    {"tr", "Turkey"},
    {"tm", "Turkmenistan"},
    {"tc", "Turks and Caicos Islands"},
    {"tv", "Tuvalu"},
    {"ug", "Uganda"},
    {"ua", "Ukraine"},
    {"ae", "United Arab Emirates"},
    {"gb", "United Kingdom"},
    {"us", "United States"},
    {"um", "United States Is."},
    {"uy", "Uruguay"},
    {"uz", "Uzbekistan"},
    {"vu", "Vanuatu"},
    {"ve", "Venezuela"},
    {"vn", "Vietnam"},
    {"vg", "Virgin Islands, British"},
    {"vi", "Virgin Islands, U.S."},
    {"wf", "Wallis and Futuna"},
    {"eh", "Western Sahara"},
    {"ye", "Yemen"},
    {"zm", "Zambia"},
    {"zw", "Zimbabwe"}
};

/* Get country name from ISO 3166 A2 */

string GetISOCountryName(const string strA2)
{
    for (int i = 0; i < LEN_TABLE_COUNTRY_CODE; i++)
    {
        if (!strA2.compare(TableCountryCode[i].strcode))
            return TableCountryCode[i].strDesc;
    }

    return "";
}


/* Language code table according to ISO 639-2 */
/* All Alpha 3 codes: "bibliographic" (B code) and "terminological" (T code) */

const struct elLanguage TableISOLanguageCode[LEN_TABLE_ISO_LANGUAGE_CODE] = {
    {"aar", "Afar"},
    {"abk", "Abkhazian"},
    {"ace", "Achinese"},
    {"ach", "Acoli"},
    {"ada", "Adangme"},
    {"ady", "Adyghe; Adygei"},
    {"afa", "Afro-Asiatic (Other)"},
    {"afh", "Afrihili"},
    {"afr", "Afrikaans"},
    {"ain", "Ainu"},
    {"aka", "Akan"},
    {"akk", "Akkadian"},
    {"alb", "Albanian"},
    {"sqi", "Albanian"},
    {"ale", "Aleut"},
    {"alg", "Algonquian languages"},
    {"alt", "Southern Altai"},
    {"amh", "Amharic"},
    {"ang", "English, Old (ca.450-1100)"},
    {"anp", "Angika"},
    {"apa", "Apache languages"},
    {"ara", "Arabic"},
    {"arc", "Aramaic"},
    {"arg", "Aragonese"},
    {"arm", "Armenian"},
    {"hye", "Armenian"},
    {"arn", "Araucanian"},
    {"arp", "Arapaho"},
    {"art", "Artificial (Other)"},
    {"arw", "Arawak"},
    {"asm", "Assamese"},
    {"ast", "Asturian; Bable"},
    {"ath", "Athapascan languages"},
    {"aus", "Australian languages"},
    {"ava", "Avaric"},
    {"ave", "Avestan"},
    {"awa", "Awadhi"},
    {"aym", "Aymara"},
    {"aze", "Azerbaijani"},
    {"bad", "Banda"},
    {"bai", "Bamileke languages"},
    {"bak", "Bashkir"},
    {"bal", "Baluchi"},
    {"bam", "Bambara"},
    {"ban", "Balinese"},
    {"baq", "Basque"},
    {"eus", "Basque"},
    {"bas", "Basa"},
    {"bat", "Baltic (Other)"},
    {"bej", "Beja"},
    {"bel", "Belarusian"},
    {"bem", "Bemba"},
    {"ben", "Bengali"},
    {"ber", "Berber (Other)"},
    {"bho", "Bhojpuri"},
    {"bih", "Bihari"},
    {"bik", "Bikol"},
    {"bin", "Bini"},
    {"bis", "Bislama"},
    {"bla", "Siksika"},
    {"bnt", "Bantu (Other)"},
    {"bos", "Bosnian"},
    {"bra", "Braj"},
    {"bre", "Breton"},
    {"btk", "Batak (Indonesia)"},
    {"bua", "Buriat"},
    {"bug", "Buginese"},
    {"bul", "Bulgarian"},
    {"bur", "Burmese"},
    {"mya", "Burmese"},
    {"byn", "Blin; Bilin"},
    {"cad", "Caddo"},
    {"cai", "Central American Indian (Other)"},
    {"car", "Carib"},
    {"cat", "Catalan; Valencian"},
    {"cau", "Caucasian (Other)"},
    {"ceb", "Cebuano"},
    {"cel", "Celtic (Other)"},
    {"cha", "Chamorro"},
    {"chb", "Chibcha"},
    {"che", "Chechen"},
    {"chg", "Chagatai"},
    {"chi", "Chinese"},
    {"zho", "Chinese"},
    {"chk", "Chuukese"},
    {"chm", "Mari"},
    {"chn", "Chinook jargon"},
    {"cho", "Choctaw"},
    {"chp", "Chipewyan"},
    {"chr", "Cherokee"},
    {"chu", "Church Slavic - Slavonic..."},
    {"chv", "Chuvash"},
    {"chy", "Cheyenne"},
    {"cmc", "Chamic languages"},
    {"cop", "Coptic"},
    {"cor", "Cornish"},
    {"cos", "Corsican"},
    {"cpe", "Creoles and pidgins, English based"},
    {"cpf", "Creoles and pidgins, French-based"},
    {"cpp", "Creoles and pidgins, Portuguese-based"},
    {"cre", "Cree"},
    {"crh", "Crimean Tatar; Crimean Turkish"},
    {"crp", "Creoles and pidgins (Other)"},
    {"csb", "Kashubian"},
    {"cus", "Cushitic (Other)"},
    {"cze", "Czech"},
    {"ces", "Czech"},
    {"dak", "Dakota"},
    {"dan", "Danish"},
    {"dar", "Dargwa"},
    {"day", "Dayak"},
    {"del", "Delaware"},
    {"den", "Slave (Athapascan)"},
    {"dgr", "Dogrib"},
    {"din", "Dinka"},
    {"div", "Divehi; Dhivehi; Maldivian"},
    {"doi", "Dogri"},
    {"dra", "Dravidian (Other)"},
    {"dsb", "Lower Sorbian"},
    {"dua", "Duala"},
    {"dum", "Dutch, Middle (ca.1050-1350)"},
    {"dut", "Dutch; Flemish"},
    {"nld", "Dutch; Flemish"},
    {"dyu", "Dyula"},
    {"dzo", "Dzongkha"},
    {"efi", "Efik"},
    {"egy", "Egyptian (Ancient)"},
    {"eka", "Ekajuk"},
    {"elx", "Elamite"},
    {"eng", "English"},
    {"enm", "English, Middle (1100-1500)"},
    {"epo", "Esperanto"},
    {"est", "Estonian"},
    {"ewe", "Ewe"},
    {"ewo", "Ewondo"},
    {"fan", "Fang"},
    {"fao", "Faroese"},
    {"fat", "Fanti"},
    {"fij", "Fijian"},
    {"fil", "Filipino; Pilipino"},
    {"fin", "Finnish"},
    {"fiu", "Finno-Ugrian (Other)"},
    {"fon", "Fon"},
    {"fre", "French"},
    {"fra", "French"},
    {"frm", "French, Middle (ca.1400-1600)"},
    {"fro", "French, Old (842-ca.1400)"},
    {"frr", "Northern Frisian"},
    {"frs", "Eastern Frisian"},
    {"fry", "Western Frisian"},
    {"ful", "Fulah"},
    {"fur", "Friulian"},
    {"gaa", "Ga"},
    {"gay", "Gayo"},
    {"gba", "Gbaya"},
    {"gem", "Germanic (Other)"},
    {"geo", "Georgian"},
    {"kat", "Georgian"},
    {"ger", "German"},
    {"deu", "German"},
    {"gez", "Geez"},
    {"gil", "Gilbertese"},
    {"gla", "Gaelic; Scottish Gaelic"},
    {"gle", "Irish"},
    {"glg", "Galician"},
    {"glv", "Manx"},
    {"gmh", "German, Middle High (ca.1050-1500)"},
    {"goh", "German, Old High (ca.750-1050)"},
    {"gon", "Gondi"},
    {"gor", "Gorontalo"},
    {"got", "Gothic"},
    {"grb", "Grebo"},
    {"grc", "Greek, Ancient (to 1453)"},
    {"gre", "Greek, Modern (1453-)"},
    {"ell", "Greek, Modern (1453-)"},
    {"grn", "Guarani"},
    {"gsw", "Alemani; Swiss German"},
    {"guj", "Gujarati"},
    {"gwi", "Gwich´in"},
    {"hai", "Haida"},
    {"hat", "Haitian; Haitian Creole"},
    {"hau", "Hausa"},
    {"haw", "Hawaiian"},
    {"heb", "Hebrew"},
    {"her", "Herero"},
    {"hil", "Hiligaynon"},
    {"him", "Himachali"},
    {"hin", "Hindi"},
    {"hit", "Hittite"},
    {"hmn", "Hmong"},
    {"hmo", "Hiri Motu"},
    {"hsb", "Upper Sorbian"},
    {"hun", "Hungarian"},
    {"hup", "Hupa"},
    {"iba", "Iban"},
    {"ibo", "Igbo"},
    {"ice", "Icelandic"},
    {"isl", "Icelandic"},
    {"ido", "Ido"},
    {"iii", "Sichuan Yi"},
    {"ijo", "Ijo"},
    {"iku", "Inuktitut"},
    {"ile", "Interlingue"},
    {"ilo", "Iloko"},
    {"ina", "Interlingua"},
    {"inc", "Indic (Other)"},
    {"ind", "Indonesian"},
    {"ine", "Indo-European (Other)"},
    {"inh", "Ingush"},
    {"ipk", "Inupiaq"},
    {"ira", "Iranian (Other)"},
    {"iro", "Iroquoian languages"},
    {"ita", "Italian"},
    {"jav", "Javanese"},
    {"jbo", "Lojban"},
    {"jpn", "Japanese"},
    {"jpr", "Judeo-Persian"},
    {"jrb", "Judeo-Arabic"},
    {"kaa", "Kara-Kalpak"},
    {"kab", "Kabyle"},
    {"kac", "Kachin"},
    {"kal", "Kalaallisut; Greenlandic"},
    {"kam", "Kamba"},
    {"kan", "Kannada"},
    {"kar", "Karen"},
    {"kas", "Kashmiri"},
    {"kau", "Kanuri"},
    {"kaw", "Kawi"},
    {"kaz", "Kazakh"},
    {"kbd", "Kabardian"},
    {"kha", "Khasi"},
    {"khi", "Khoisan (Other)"},
    {"khm", "Khmer"},
    {"kho", "Khotanese"},
    {"kik", "Kikuyu; Gikuyu"},
    {"kin", "Kinyarwanda"},
    {"kir", "Kirghiz"},
    {"kmb", "Kimbundu"},
    {"kok", "Konkani"},
    {"kom", "Komi"},
    {"kon", "Kongo"},
    {"kor", "Korean"},
    {"kos", "Kosraean"},
    {"kpe", "Kpelle"},
    {"krc", "Karachay-Balkar"},
    {"krl", "Karelian"},
    {"kro", "Kru"},
    {"kru", "Kurukh"},
    {"kua", "Kuanyama; Kwanyama"},
    {"kum", "Kumyk"},
    {"kur", "Kurdish"},
    {"kut", "Kutenai"},
    {"lad", "Ladino"},
    {"lah", "Lahnda"},
    {"lam", "Lamba"},
    {"lao", "Lao"},
    {"lat", "Latin"},
    {"lav", "Latvian"},
    {"lez", "Lezghian"},
    {"lim", "Limburgan; Limburger; Limburgish"},
    {"lin", "Lingala"},
    {"lit", "Lithuanian"},
    {"lol", "Mongo"},
    {"loz", "Lozi"},
    {"ltz", "Luxembourgish; Letzeburgesch"},
    {"lua", "Luba-Lulua"},
    {"lub", "Luba-Katanga"},
    {"lug", "Ganda"},
    {"lui", "Luiseno"},
    {"lun", "Lunda"},
    {"luo", "Luo (Kenya and Tanzania)"},
    {"lus", "lushai"},
    {"mac", "Macedonian"},
    {"mkd", "Macedonian"},
    {"mad", "Madurese"},
    {"mag", "Magahi"},
    {"mah", "Marshallese"},
    {"mai", "Maithili"},
    {"mak", "Makasar"},
    {"mal", "Malayalam"},
    {"man", "Mandingo"},
    {"mao", "Maori"},
    {"mri", "Maori"},
    {"map", "Austronesian (Other)"},
    {"mar", "Marathi"},
    {"mas", "Masai"},
    {"may", "Malay"},
    {"msa", "Malay"},
    {"mdf", "Moksha"},
    {"mdr", "Mandar"},
    {"men", "Mende"},
    {"mga", "Irish, Middle (900-1200)"},
    {"mic", "Mi'kmaq; Micmac"},
    {"min", "Minangkabau"},
    {"mis", "Miscellaneous languages"},
    {"mkh", "Mon-Khmer (Other)"},
    {"mlg", "Malagasy"},
    {"mlt", "Maltese"},
    {"mnc", "Manchu"},
    {"mni", "Manipuri"},
    {"mno", "Manobo languages"},
    {"moh", "Mohawk"},
    {"mol", "Moldavian"},
    {"mon", "Mongolian"},
    {"mos", "Mossi"},
    {"mul", "Multiple languages"},
    {"mun", "Munda languages"},
    {"mus", "Creek"},
    {"mwl", "Mirandese"},
    {"mwr", "Marwari"},
    {"myn", "Mayan languages"},
    {"myv", "Erzya"},
    {"nah", "Nahuatl"},
    {"nai", "North American Indian"},
    {"nap", "Neapolitan"},
    {"nau", "Nauru"},
    {"nav", "Navajo; Navaho"},
    {"nbl", "Ndebele, South; South Ndebele"},
    {"nde", "Ndebele, North; North Ndebele"},
    {"ndo", "Ndonga"},
    {"nds", "Low German; Low Saxon..."},
    {"nep", "Nepali"},
    {"new", "Nepal Bhasa; Newari"},
    {"nia", "Nias"},
    {"nic", "Niger-Kordofanian (Other)"},
    {"niu", "Niuean"},
    {"nno", "Norwegian Nynorsk"},
    {"nob", "Norwegian Bokmĺl"},
    {"nog", "Nogai"},
    {"non", "Norse, Old"},
    {"nor", "Norwegian"},
    {"nqo", "N'ko"},
    {"nso", "Northern Sotho; Pedi; Sepedi"},
    {"nub", "Nubian languages"},
    {"nwc", "Classical Newari; Classical Nepal Bhasa"},
    {"nya", "Chichewa; Chewa; Nyanja"},
    {"nym", "Nyamwezi"},
    {"nyn", "Nyankole"},
    {"nyo", "Nyoro"},
    {"nzi", "Nzima"},
    {"oci", "Occitan (post 1500); Provençal"},
    {"oji", "Ojibwa"},
    {"ori", "Oriya"},
    {"orm", "Oromo"},
    {"osa", "Osage"},
    {"oss", "Ossetian; Ossetic"},
    {"ota", "Turkish, Ottoman (1500-1928)"},
    {"oto", "Otomian languages"},
    {"paa", "Papuan (Other)"},
    {"pag", "Pangasinan"},
    {"pal", "Pahlavi"},
    {"pam", "Pampanga"},
    {"pan", "Panjabi; Punjabi"},
    {"pap", "Papiamento"},
    {"pau", "Palauan"},
    {"peo", "Persian, Old (ca.600-400 B.C.)"},
    {"per", "Persian"},
    {"fas", "Persian"},
    {"phi", "Philippine (Other)"},
    {"phn", "Phoenician"},
    {"pli", "Pali"},
    {"pol", "Polish"},
    {"pon", "Pohnpeian"},
    {"por", "Portuguese"},
    {"pra", "Prakrit languages"},
    {"pro", "Provençal, Old (to 1500)"},
    {"pus", "Pushto"},
    {"que", "Quechua"},
    {"raj", "Rajasthani"},
    {"rap", "Rapanui"},
    {"rar", "Rarotongan"},
    {"roa", "Romance (Other)"},
    {"roh", "Raeto-Romance"},
    {"rom", "Romany"},
    {"rum", "Romanian"},
    {"ron", "Romanian"},
    {"run", "Rundi"},
    {"rup", "Aromanian; Arumanian; Macedo-Romanian"},
    {"rus", "Russian"},
    {"sad", "Sandawe"},
    {"sag", "Sango"},
    {"sah", "Yakut"},
    {"sai", "South American Indian (Other)"},
    {"sal", "Salishan languages"},
    {"sam", "Samaritan Aramaic"},
    {"san", "Sanskrit"},
    {"sas", "Sasak"},
    {"sat", "Santali"},
    {"scc", "Serbian"},
    {"srp", "Serbian"},
    {"scn", "Sicilian"},
    {"sco", "Scots"},
    {"scr", "Croatian"},
    {"hrv", "Croatian"},
    {"sel", "Selkup"},
    {"sem", "Semitic (Other)"},
    {"sga", "Irish, Old (to 900)"},
    {"sgn", "Sign Languages"},
    {"shn", "Shan"},
    {"sid", "Sidamo"},
    {"sin", "Sinhala; Sinhalese"},
    {"sio", "Siouan languages"},
    {"sit", "Sino-Tibetan (Other)"},
    {"sla", "Slavic (Other)"},
    {"slo", "Slovak"},
    {"slk", "Slovak"},
    {"slv", "Slovenian"},
    {"sma", "Southern Sami"},
    {"sme", "Northern Sami"},
    {"smi", "Sami languages (Other)"},
    {"smj", "Lule Sami"},
    {"smn", "Inari Sami"},
    {"smo", "Samoan"},
    {"sms", "Skolt Sami"},
    {"sna", "Shona"},
    {"snd", "Sindhi"},
    {"snk", "Soninke"},
    {"sog", "Sogdian"},
    {"som", "Somali"},
    {"son", "Songhai"},
    {"sot", "Sotho, Southern"},
    {"spa", "Spanish; Castilian"},
    {"srd", "Sardinian"},
    {"srn", "Sranan Togo"},
    {"srr", "Serer"},
    {"ssa", "Nilo-Saharan (Other)"},
    {"ssw", "Swati"},
    {"suk", "Sukuma"},
    {"sun", "Sundanese"},
    {"sus", "Susu"},
    {"sux", "Sumerian"},
    {"swa", "Swahili"},
    {"swe", "Swedish"},
    {"syr", "Syriac"},
    {"tah", "Tahitian"},
    {"tai", "Tai (Other)"},
    {"tam", "Tamil"},
    {"tat", "Tatar"},
    {"tel", "Telugu"},
    {"tem", "Timne"},
    {"ter", "Tereno"},
    {"tet", "Tetum"},
    {"tgk", "Tajik"},
    {"tgl", "Tagalog"},
    {"tha", "Thai"},
    {"tib", "Tibetan"},
    {"bod", "Tibetan"},
    {"tig", "Tigre"},
    {"tir", "Tigrinya"},
    {"tiv", "Tiv"},
    {"tkl", "Tokelau"},
    {"tlh", "Klingon; tlhIngan-Hol"},
    {"tli", "Tlingit"},
    {"tmh", "Tamashek"},
    {"tog", "Tonga (Nyasa)"},
    {"ton", "Tonga (Tonga Islands)"},
    {"tpi", "Tok Pisin"},
    {"tsi", "Tsimshian"},
    {"tsn", "Tswana"},
    {"tso", "Tsonga"},
    {"tuk", "Turkmen"},
    {"tum", "Tumbuka"},
    {"tup", "Tupi languages"},
    {"tur", "Turkish"},
    {"tut", "Altaic (Other)"},
    {"tvl", "Tuvalu"},
    {"twi", "Twi"},
    {"tyv", "Tuvinian"},
    {"udm", "Udmurt"},
    {"uga", "Ugaritic"},
    {"uig", "Uighur; Uyghur"},
    {"ukr", "Ukrainian"},
    {"umb", "Umbundu"},
    {"und", "Undetermined"},
    {"urd", "Urdu"},
    {"uzb", "Uzbek"},
    {"vai", "Vai"},
    {"ven", "Venda"},
    {"vie", "Vietnamese"},
    {"vol", "Volapük"},
    {"vot", "Votic"},
    {"wak", "Wakashan languages"},
    {"wal", "Walamo"},
    {"war", "Waray"},
    {"was", "Washo"},
    {"wel", "Welsh"},
    {"cym", "Welsh"},
    {"wen", "Sorbian languages"},
    {"wln", "Walloon"},
    {"wol", "Wolof"},
    {"xal", "Kalmyk; Oirat"},
    {"xho", "Xhosa"},
    {"yao", "Yao"},
    {"yap", "Yapese"},
    {"yid", "Yiddish"},
    {"yor", "Yoruba"},
    {"ypk", "Yupik languages"},
    {"zap", "Zapotec"},
    {"zen", "Zenaga"},
    {"zha", "Zhuang; Chuang"},
    {"znd", "Zande"},
    {"zul", "Zulu"},
    {"zun", "Zuni"},
    {"zxx", "No linguistic content"},
    {"zza", "Zaza; Dimili; Dimli; Kirdki..."}
};

/* Get language name from ISO */

string GetISOLanguageName(const string strA3)
{
    for (int i = 0; i < LEN_TABLE_ISO_LANGUAGE_CODE; i++)
    {
        if (!strA3.compare(TableISOLanguageCode[i].strISOCode))
            return TableISOLanguageCode[i].strDesc;
    }

    return "";
}


/* CIRAF zones */

const string strTableCIRAFzones[LEN_TABLE_CIRAF_ZONES] = {
    "", /* 0 undefined */
    "Alaska", /* 1 */
    "west Canada", /* 2 */
    "central Canada - west", /* 3 */
    "central Canada - east, Baffin Island", /* 4 */
    "Greenland", /* 5 */
    "west USA", /* 6 */
    "central USA", /* 7 */
    "east USA", /* 8 */
    "east Canada", /* 9 */
    "Belize, Guatemala, Mexico", /* 10 */
    "Caribbean, central America",  /* 11 */
    "northwestern south America", /* 12 */
    "northeast Brazil", /* 13 */
    "southwestern south America", /* 14 */
    "southeast Brazil",  /* 15 */
    "south Argentina, south Chile, Falkland Islands", /* 16 */
    "Iceland", /* 17 */
    "Scandinavia", /* 18 */
    "west Russia northwest", /* 19 */
    "west Russia north", /* 20 */
    "central Russia northwest", /* 21 */
    "central Russia north", /* 22 */
    "central Russia east", /* 23 */
    "east Russia northwest", /* 24 */
    "east Russia north", /* 25 */
    "east Russia northeast", /* 26 */
    "northwest Europe", /* 27 */
    "central east south Europe", /* 28 */
    "Baltics and west Russia", /* 29 */
    "central Asia, west Russia southeast", /* 30 */
    "central Russia southwest, east Kazakhstan, east Kyrgyzstan", /* 31 */
    "central Russia south, west Mongolia", /* 32 */
    "central Russia southeast, east Mongolia", /* 33 */
    "east Russia southwest: Sakhalin, Sikhote Alin", /* 34 */
    "east Russia east: Kamchatka", /* 35 */
    "Azores, Canary Island, Madeira", /* 36 */
    "southwest Europe, northwest Africa", /* 37 */
    "Egypt, Libya", /* 38 */
    "Middle East", /* 39 */
    "Afghanistan, Iran", /* 40 */
    "Bangladesh, Bhutan, India, Nepal, Pakistan", /* 41 */
    "west China", /* 42 */
    "central China", /* 43 */
    "east China, Macao, Hong Kong, North Korea, South Korea, Taiwan", /* 44 */
    "Japan", /* 45 */
    "west Africa", /* 46 */
    "west Sudan", /* 47 */
    "Horn of Africa", /* 48 */
    "Kampuchea, Laos, Myanmar, Vietnam", /* 49 */
    "Philippines", /* 50 */
    "Malaysia, Papua New Guinea, west Indonesia", /* 51 */
    "Angola, Burundi, Congo, Gabon, Zaire", /* 52 */
    "Madagascar, Malawi, Mozambique, Seychelles, Zambia, Zimbabwe", /* 53 */
    "Malaysia, Singapore, west Indonesia", /* 54 */
    "northeast Australia", /* 55 */
    "Caledonia, Fiji/Vanuatu", /* 56 */
    "Botswana, Lesotho, Namibia, Swaziland, South African Republic", /* 57 */
    "west Australia", /* 58 */
    "southeast Australia", /* 59 */
    "New Zealand", /* 60 */
    "Hawaii", /* 61 */
    "Phoenix Islands, Samoa", /* 62 */
    "Cook Islands, Polynesia", /* 63 */
    "Guam/Palau, Saipan", /* 64 */
    "Kiribati, Marshall", /* 65 */
    "central Atlantic - south: Ascension, St. Helena", /* 66 */
    "Antarctica", /* 67 */
    "southwest Indian Ocean: Kerguelen", /* 68 */
    "Antarctica", /* 69 */
    "Antarctica", /* 70 */
    "Antarctica", /* 71 */
    "Antarctica", /* 72 */
    "Antarctica", /* 73 */
    "South Pole", /* 74 */
    "North Pole", /* 75 */
    "northeast Pacific", /* 76 */
    "central Pacific - northeast", /* 77 */
    "central Pacific - southeast", /* 78 */
    "central Indian Ocean", /* 79 */
    "northern Atlantic", /* 80 */
    "central Atlantic", /* 81 */
    "northwest Pacific", /* 82 */
    "south Pacific", /* 83 */
    "south Atlantic", /* 84 */
    "southeast Indian Ocean" /* 85 */
};
