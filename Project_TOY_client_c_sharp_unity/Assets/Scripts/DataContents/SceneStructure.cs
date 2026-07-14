using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

[Serializable]
public class SceneStructure
{
    public int id;
    public string name;
    public string type;
}


[Serializable]
public class SceneStructureData : ILoader<int, SceneStructure>
{
    public List<SceneStructure> sceneStructures = new List<SceneStructure>();

    public Dictionary<int, SceneStructure> MakeDict()
    {
        Dictionary<int, SceneStructure> dict = new Dictionary<int, SceneStructure>();
        foreach (SceneStructure sceneStructure in sceneStructures)
            dict.Add(sceneStructure.id, sceneStructure);
        return dict;
    }
}
